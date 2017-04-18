/*
 * ResonanzEngine.cpp
 *
 *  Created on: 13.6.2015
 *      Author: Tomas Ukkonen
 */

#include "ResonanzEngine.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <map>

#include <math.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

#include <dinrhiw.h>

#include "Log.h"
#include "NMCFile.h"

#include "NoEEGDevice.h"
#include "RandomEEG.h"

#ifdef LIGHTSTONE
#include "LightstoneDevice.h"
#endif

#ifdef EMOTIV_INSIGHT
#include "EmotivInsightStub.h"
#include "EmotivInsightPipeServer.h"
#endif

#include "MuseOSC.h"

#include "FMSoundSynthesis.h"

#include "SDLTheora.h"

#include "hermitecurve.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace whiteice {
namespace resonanz {


ResonanzEngine::ResonanzEngine()
{        
        logging.info("ResonanzEngine ctor starting");
	
	std::lock_guard<std::mutex> lock(thread_mutex);

	logging.info("ResonanzEngine() ctor started");

	// initializes random number generation here (again) this is needed?
	// so that JNI implementation gets different random numbers and experiments don't repeat each other..
	
	srand(rng.rand()); // initializes using RDRAND if availabe
	                   // otherwise uses just another value from rand()
	
	engine_setStatus("resonanz-engine: starting..");
	
	video = nullptr;
	eeg   = nullptr;
	synth = nullptr;
	mic   = nullptr;

	workerThread = nullptr;
	thread_is_running = true;

	{
		std::lock_guard<std::mutex> lock(command_mutex);
		incomingCommand = nullptr;
		currentCommand.command = ResonanzCommand::CMD_DO_NOTHING;
		currentCommand.showScreen = false;
	}

	{
	        std::lock_guard<std::mutex> lock(eeg_mutex);
	        eeg = new NoEEGDevice();
	        eegDeviceType = ResonanzEngine::RE_EEG_NO_DEVICE;
		
		std::vector<unsigned int> nnArchitecture;
		nnArchitecture.push_back(eeg->getNumberOfSignals());
		nnArchitecture.push_back(neuralnetwork_complexity*eeg->getNumberOfSignals());
		nnArchitecture.push_back(neuralnetwork_complexity*eeg->getNumberOfSignals());
		nnArchitecture.push_back(eeg->getNumberOfSignals());
		
		nn = new whiteice::nnetwork<>(nnArchitecture);
		nn->setNonlinearity(nn->getLayers()-1, whiteice::nnetwork<>::pureLinear);
		
		// creates dummy synth neural network
		const int synth_number_of_parameters = 3;
		
		nnArchitecture.clear();
		nnArchitecture.push_back(eeg->getNumberOfSignals() + 2*synth_number_of_parameters);
		nnArchitecture.push_back(neuralnetwork_complexity*(eeg->getNumberOfSignals() + 2*synth_number_of_parameters));
		nnArchitecture.push_back(neuralnetwork_complexity*(eeg->getNumberOfSignals() + 2*synth_number_of_parameters));
		nnArchitecture.push_back(eeg->getNumberOfSignals());
		
		nnsynth = new whiteice::nnetwork<>(nnArchitecture);
		nnsynth->setNonlinearity(nnsynth->getLayers()-1, whiteice::nnetwork<>::pureLinear);
	}
	
	
	thread_initialized = false;

	// starts updater thread thread
	workerThread = new std::thread(&ResonanzEngine::engine_loop, this);
	workerThread->detach();

#ifndef _WIN32	
	// for some reason this leads to DEADLOCK on Windows ???
	
	// waits for thread to initialize itself properly
	while(thread_initialized == false){
	  logging.info("ResonanzEngine ctor waiting worker thread to init");
	  std::chrono::milliseconds duration(1000); // 1000ms (thread sleep/working period is something like < 100ms)
	  std::this_thread::sleep_for(duration);
	}
#endif

	logging.info("ResonanzEngine ctor finished");
}

ResonanzEngine::~ResonanzEngine()
{
	std::lock_guard<std::mutex> lock(thread_mutex);

	engine_setStatus("resonanz-engine: shutdown..");

	thread_is_running = false;
	if(workerThread == nullptr)
		return; // no thread is running

	// waits for thread to stop
	std::chrono::milliseconds duration(1000); // 1000ms (thread sleep/working period is something like < 100ms)
	std::this_thread::sleep_for(duration);

	// deletes thread whether it is still running or not
	delete workerThread;
	workerThread = nullptr;

	if(eeg != nullptr){
	        std::lock_guard<std::mutex> lock(eeg_mutex);
		delete eeg;
		eeg = nullptr;
	}

	engine_setStatus("resonanz-engine: halted");
}


// what resonanz is doing right now [especially interesting if we are optimizing model]
std::string ResonanzEngine::getEngineStatus() throw()
{
	std::lock_guard<std::mutex> lock(status_mutex);
	return engineState;
}

// resets resonanz-engine (worker thread stop and recreation)
bool ResonanzEngine::reset() throw()
{
	try{
		std::lock_guard<std::mutex> lock(thread_mutex);

		engine_setStatus("resonanz-engine: restarting..");

		if(thread_is_running || workerThread != nullptr){ // thread appears to be running
			thread_is_running = false;

			// waits for thread to stop
			std::chrono::milliseconds duration(1000); // 1000ms (thread sleep/working period is something like < 100ms)
			std::this_thread::sleep_for(duration);

			// deletes thread whether it is still running or not
			delete workerThread;
		}

		{
			std::lock_guard<std::mutex> lock(command_mutex);
			if(incomingCommand != nullptr) delete incomingCommand;
			incomingCommand = nullptr;
			currentCommand.command = ResonanzCommand::CMD_DO_NOTHING;
			currentCommand.showScreen = false;
		}

		workerThread = nullptr;
		thread_is_running = true;

		// starts updater thread thread
		workerThread = new std::thread(&ResonanzEngine::engine_loop, this);
		workerThread->detach();

		return true;
	}
	catch(std::exception& e){ return false; }
}

bool ResonanzEngine::cmdDoNothing(bool showScreen)
{
        std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_NOTHING;
	incomingCommand->showScreen = showScreen;
	incomingCommand->pictureDir = "";
	incomingCommand->keywordsFile = "";
	incomingCommand->modelDir = "";

	return true;
}


bool ResonanzEngine::cmdRandom(const std::string& pictureDir, const std::string& keywordsFile,
			       const std::string& audioFile,
			       bool saveVideo) throw()
{
	if(pictureDir.length() <= 0 || keywordsFile.length() <= 0)
		return false;

	// TODO check that those directories and files actually exist

	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_RANDOM;
	incomingCommand->showScreen = true;
	incomingCommand->pictureDir = pictureDir;
	incomingCommand->keywordsFile = keywordsFile;
	incomingCommand->modelDir = "";
	incomingCommand->saveVideo = saveVideo;
	incomingCommand->audioFile = audioFile;
	

	return true;
}


bool ResonanzEngine::cmdMeasure(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir) throw()
{
	if(pictureDir.length() <= 0 || keywordsFile.length() <= 0 || modelDir.length() <= 0)
		return false;

	// TODO check that those directories and files actually exist

	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_MEASURE;
	incomingCommand->showScreen = true;
	incomingCommand->pictureDir = pictureDir;
	incomingCommand->keywordsFile = keywordsFile;
	incomingCommand->modelDir = modelDir;

	return true;
}


bool ResonanzEngine::cmdOptimizeModel(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir) throw()
{
	if(modelDir.length() <= 0)
		return false;

	// TODO check that those directories and files actually exist

	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_OPTIMIZE;
	incomingCommand->showScreen = false;
	incomingCommand->pictureDir = pictureDir;
	incomingCommand->keywordsFile = keywordsFile;
	incomingCommand->modelDir = modelDir;

	return true;
}


bool ResonanzEngine::cmdMeasureProgram(const std::string& mediaFile,
			const std::vector<std::string>& signalNames,
			const unsigned int programLengthTicks) throw()
{
	// could do more checks here but JNI code calling this SHOULD WORK CORRECTLY SO I DON'T

	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_MEASURE_PROGRAM;
	incomingCommand->showScreen = true;
	incomingCommand->audioFile = mediaFile;
	incomingCommand->signalName = signalNames;
	incomingCommand->blindMonteCarlo = false;
	incomingCommand->saveVideo = false;
	incomingCommand->programLengthTicks = programLengthTicks;

	return true;
}


bool ResonanzEngine::cmdExecuteProgram(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir,
		const std::string& audioFile,
		const std::vector<std::string>& targetSignal, const std::vector< std::vector<float> >& program,
		bool blindMonteCarlo, bool saveVideo) throw()
{
	if(targetSignal.size() != program.size())
		return false;

	if(targetSignal.size() <= 0)
		return false;

	for(unsigned int i=0;i<targetSignal.size();i++){
		if(targetSignal[i].size() <= 0)
			return false;

		if(program[i].size() <= 0)
			return false;

		if(program[i].size() != program[0].size())
			return false;
	}

	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	// interpolation of missing (negative) values between value points:
	// uses NMCFile functionality for this

	auto programcopy = program;

	for(auto& p : programcopy)
		NMCFile::interpolateProgram(p);

	incomingCommand->command = ResonanzCommand::CMD_DO_EXECUTE;
	incomingCommand->showScreen = true;
	incomingCommand->pictureDir = pictureDir;
	incomingCommand->keywordsFile = keywordsFile;
	incomingCommand->modelDir = modelDir;
	incomingCommand->audioFile = audioFile;
	incomingCommand->signalName = targetSignal;
	incomingCommand->programValues = programcopy;
	incomingCommand->blindMonteCarlo = blindMonteCarlo;
	incomingCommand->saveVideo = saveVideo;

	return true;
}



bool ResonanzEngine::cmdStopCommand() throw()
{
	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_NOTHING;
	incomingCommand->showScreen = false;
	incomingCommand->pictureDir = "";
	incomingCommand->keywordsFile = "";
	incomingCommand->modelDir = "";

	return true;
}


bool ResonanzEngine::isBusy() throw()
{
	if(currentCommand.command == ResonanzCommand::CMD_DO_NOTHING){
		if(incomingCommand != nullptr)
			return true; // there is incoming work to be processed
		else
			return false;
	}
	else{
		return true;
	}
}


/**
 * has a key been pressed since the latest check?
 *
 */
bool ResonanzEngine::keypress(){
	std::lock_guard<std::mutex> lock(keypress_mutex);
	if(keypressed){
		keypressed = false;
		return true;
	}
	else return false;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


bool ResonanzEngine::setEEGDeviceType(int deviceNumber)
{
         std::lock_guard<std::mutex> lock1(eeg_mutex);
	 
         try{
           	if(eegDeviceType == deviceNumber)
			return true; // nothing to do

		std::lock_guard<std::mutex> lock2(command_mutex);

		if(currentCommand.command != ResonanzCommand::CMD_DO_NOTHING)
			return false; // can only change EEG in inactive state

		if(deviceNumber == ResonanzEngine::RE_EEG_NO_DEVICE){
			if(eeg != nullptr) delete eeg;
			eeg = new NoEEGDevice();
		}
		else if(deviceNumber == ResonanzEngine::RE_EEG_RANDOM_DEVICE){
			if(eeg != nullptr) delete eeg;
			eeg = new RandomEEG();
		}
#ifdef EMOTIV_INSIGHT
		else if(deviceNumber == ResonanzEngine::RE_EEG_EMOTIV_INSIGHT_DEVICE){
			if(eeg != nullptr) delete eeg;
			eeg = new EmotivInsightPipeServer("\\\\.\\pipe\\emotiv-insight-data");
		}
#endif
		else if(deviceNumber == ResonanzEngine::RE_EEG_IA_MUSE_DEVICE){
			if(eeg != nullptr) delete eeg;
			eeg = new MuseOSC(4545);
		}
#ifdef LIGHTSTONE
		else if(deviceNumber == ResonanzEngine::RE_WD_LIGHTSTONE){
			if(eeg != nullptr) delete eeg;
			eeg = new LightstoneDevice();
		}
#endif
		else{
			return false; // unknown device
		}

		// updates neural network model according to signal numbers of the EEG device
		{
			std::vector<unsigned int> nnArchitecture;
			nnArchitecture.push_back(eeg->getNumberOfSignals());
			nnArchitecture.push_back(neuralnetwork_complexity*eeg->getNumberOfSignals());
			nnArchitecture.push_back(neuralnetwork_complexity*eeg->getNumberOfSignals());
			nnArchitecture.push_back(eeg->getNumberOfSignals());

			if(nn != nullptr) delete nn;

			nn = new whiteice::nnetwork<>(nnArchitecture);
			nn->setNonlinearity(nn->getLayers()-1,
					    whiteice::nnetwork<>::pureLinear);
						
			
			nnArchitecture.clear();
			
			if(synth){
			  // nnsynth(synthBefore, synthProposed, currentEEG) = dEEG/dt (predictedEEG = currentEEG + dEEG/dT * TIMESTEP)
			  
			  nnArchitecture.push_back(eeg->getNumberOfSignals() + 
						   2*synth->getNumberOfParameters());
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth->getNumberOfParameters()));
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth->getNumberOfParameters()));
			  nnArchitecture.push_back(eeg->getNumberOfSignals());
			  
			  if(nnsynth != nullptr) delete nnsynth;
		
			  nnsynth = new whiteice::nnetwork<>(nnArchitecture);
			  nnsynth->setNonlinearity(nnsynth->getLayers()-1,
						   whiteice::nnetwork<>::pureLinear);
			}
			else{
			  const int synth_number_of_parameters = 6;
			  
			  nnArchitecture.push_back(eeg->getNumberOfSignals() + 
						   2*synth_number_of_parameters);
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth_number_of_parameters));
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth_number_of_parameters));
			  nnArchitecture.push_back(eeg->getNumberOfSignals());
			  
			  if(nnsynth != nullptr) delete nnsynth;
			  
			  nnsynth = new whiteice::nnetwork<>(nnArchitecture);
			  nnsynth->setNonlinearity(nnsynth->getLayers()-1,
						   whiteice::nnetwork<>::pureLinear);
			}
		}

		eegDeviceType = deviceNumber;

		return true;
	}
	catch(std::exception& e){
		std::string error = "setEEGDeviceType() internal error: ";
		error += e.what();

		logging.warn(error);

		eegDeviceType = ResonanzEngine::RE_EEG_NO_DEVICE;
		eeg = new NoEEGDevice();
		
		{
			std::vector<unsigned int> nnArchitecture;
			nnArchitecture.push_back(eeg->getNumberOfSignals());
			nnArchitecture.push_back(neuralnetwork_complexity*eeg->getNumberOfSignals());
			nnArchitecture.push_back(neuralnetwork_complexity*eeg->getNumberOfSignals());
			nnArchitecture.push_back(eeg->getNumberOfSignals());

			if(nn != nullptr) delete nn;

			nn = new whiteice::nnetwork<>(nnArchitecture);
			nn->setNonlinearity(nn->getLayers()-1,
					    whiteice::nnetwork<>::pureLinear);
			
			nnArchitecture.clear();
			
			if(synth){			  
			  nnArchitecture.push_back(eeg->getNumberOfSignals() + 
						   2*synth->getNumberOfParameters());
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth->getNumberOfParameters()));
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth->getNumberOfParameters()));
			  nnArchitecture.push_back(eeg->getNumberOfSignals());
			  
			  if(nnsynth != nullptr) delete nnsynth;
		
			  nnsynth = new whiteice::nnetwork<>(nnArchitecture);
			  nnsynth->setNonlinearity(nnsynth->getLayers()-1,
						   whiteice::nnetwork<>::pureLinear);
			}
			else{
			  const int synth_number_of_parameters = 6;
			  
			  nnArchitecture.push_back(eeg->getNumberOfSignals() + 
						   2*synth_number_of_parameters);
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth_number_of_parameters));
			  nnArchitecture.push_back(neuralnetwork_complexity*
						   (eeg->getNumberOfSignals() + 
						    2*synth_number_of_parameters));
			  nnArchitecture.push_back(eeg->getNumberOfSignals());
			  
			  if(nnsynth != nullptr) delete nnsynth;
			  
			  nnsynth = new whiteice::nnetwork<>(nnArchitecture);
			  nnsynth->setNonlinearity(nnsynth->getLayers()-1,
						   whiteice::nnetwork<>::pureLinear);
			}
		}
		
		return false;
	}
}


int ResonanzEngine::getEEGDeviceType()
{
	std::lock_guard<std::mutex> lock(eeg_mutex);

	if(eeg != nullptr)
		return eegDeviceType;
	else
		return RE_EEG_NO_DEVICE;
}


const DataSource& ResonanzEngine::getDevice() const
{
        assert(eeg != nullptr);
  
        return (*eeg);
}


void ResonanzEngine::getEEGDeviceStatus(std::string& status)
{
	std::lock_guard<std::mutex> lock(eeg_mutex);

	if(eeg != nullptr){
		if(eeg->connectionOk()){
			std::vector<float> values;
			eeg->data(values);

			if(values.size() > 0){
				status = "Device is connected.\n";

				status = status + "Latest measurements: ";

				char buffer[80];
				for(unsigned int i=0;i<values.size();i++){
					snprintf(buffer, 80, "%.2f ", values[i]);
					status = status + buffer;
				}

				status = status + ".";
			}
			else{
				status = "Device is NOT connected.";
			}
		}
		else{
			status = "Device is NOT connected.";
		}
	}
	else{
		status = "No device.";
	}
}


bool ResonanzEngine::setParameter(const std::string& parameter, const std::string& value)
{
	{
		char buffer[256];
		snprintf(buffer, 256, "resonanz-engine::setParameter: %s = %s", parameter.c_str(), value.c_str());
		logging.info(buffer);
	}

	if(parameter == "pca-preprocess"){
		if(value == "true"){
			pcaPreprocess = true;
			return true;
		}
		else if(value == "false"){
			pcaPreprocess = false;
			return true;
		}
		else return false;

	}
	else if(parameter == "use-bayesian-nnetwork"){
		if(value == "true"){
			use_bayesian_nnetwork = true;
			return true;
		}
		else if(value == "false"){
			use_bayesian_nnetwork = false;
			return true;
		}
		else return false;
	}
	else if(parameter == "use-data-rbf"){
		if(value == "true"){
			dataRBFmodel = true;
			return true;
		}
		else if(value == "false"){
			dataRBFmodel = false;
			return true;
		}
		else return false;
	}
	else if(parameter == "optimize-synth-only"){
		if(value == "true"){
			optimizeSynthOnly = true;
			return true;
		}
		else if(value == "false"){
			optimizeSynthOnly = false;
			return true;
		}
		else return false;
	}
	else if(parameter == "fullscreen"){
	        if(value == "true"){
		        fullscreen = true;
			return true;
		}
		else if(value == "false"){
		        fullscreen = false;
			return true;
		}
		else return false;
	}
	else if(parameter == "loop"){
	        if(value == "true"){
		        loopMode = true;
			return true;
		}
		else if(value == "false"){
		        loopMode = false;
			return true;
		}
		else return false;
	}
	else{
		return false;
	}

	return false;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// main worker thread loop to execute commands

void ResonanzEngine::engine_loop()
{
        logging.info("engine_loop() started");


#ifdef _WIN32
	{
		// set process priority
		logging.info("windows os: setting resonanz thread high priority");
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}
#endif

	long long tickStartTime = 0;
	{
		auto t1 = std::chrono::system_clock::now().time_since_epoch();
		auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

		tickStartTime = t1ms;
	}

	long long lastTickProcessed = -1;
	tick = 0;
	
	long long eegLastTickConnectionOk = tick;

	// later autodetected to good values based on display screen resolution
	SCREEN_WIDTH  = 800;
	SCREEN_HEIGHT = 600;

	const std::string fontname = "Vera.ttf";

	bnn = new bayesian_nnetwork<>();

	unsigned int currentPictureModel = 0;
	unsigned int currentKeywordModel = 0;
	bool soundModelCalculated = false;

	// model optimization ETA information
	whiteice::linear_ETA<float> optimizeETA;

	// used to execute program [targetting target values]
	const float programHz = 1.0; // 1 program step means 1 second
	std::vector< std::vector<float> > program;
	std::vector< std::vector<float> > programVar;
	programStarted = 0LL; // program has not been started
	long long lastProgramSecond = 0LL;
	unsigned int eegConnectionDownTime = 0;

	std::vector<float> eegCurrent;

	
	// thread has started successfully
	thread_initialized = true;
	
	
	// tries to initialize SDL library functionality - and load the font
	{
		bool initialized = false;

		while(thread_is_running){
			try{
				if(engine_SDL_init(fontname)){
					initialized = true;
					break;
 				}
			}
			catch(std::exception& e){ }

			engine_setStatus("resonanz-engine: re-trying to initialize graphics..");
			engine_sleep(1000);
		}

		if(thread_is_running == false){
			if(initialized) engine_SDL_deinit();
			thread_initialized = true; // should never happen/be needed..
			return;
		}
	}
	



	while(thread_is_running){

		// sleeps until there is a new engine tick
		while(lastTickProcessed >= tick){
			auto t1 = std::chrono::system_clock::now().time_since_epoch();
			auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

			auto currentTick = (t1ms - tickStartTime)/TICK_MS;

			if(tick < currentTick)
			 	tick = currentTick;
			else
				engine_sleep(TICK_MS/10);
		}

		lastTickProcessed = tick;
		
		
		
		ResonanzCommand prevCommand = currentCommand;
		if(engine_checkIncomingCommand() == true){
			logging.info("new engine command received");
			// we must make engine state transitions, state transfer from the previous command to the new command

			// state exit actions:
			if(prevCommand.command == ResonanzCommand::CMD_DO_RANDOM){
			  // stop playing sound
			  if(synth){
			    logging.info("stop synth");
			    synth->pause();
			    synth->reset();
			  }

			  // stop playing audio if needed
			  if(prevCommand.audioFile.length() > 0){
			    logging.info("stop audio file playback");
			    engine_stopAudioFile();
			  }

			  // stops encoding if needed
			  if(video != nullptr){
			    auto t1 = std::chrono::system_clock::now().time_since_epoch();
			    auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

			    logging.info("stopping theora video encoding.");
			    
			    video->stopEncoding(t1ms - programStarted);
			    delete video;
			    video = nullptr;
			    programStarted = 0;
			  }
			  
			}
			else if(prevCommand.command == ResonanzCommand::CMD_DO_MEASURE){
			        // stop playing sound
			        if(synth){
				  synth->pause();
				  synth->reset();
				  logging.info("stop synth");
			        }
			  
			        engine_setStatus("resonanz-engine: saving database..");
				if(engine_saveDatabase(prevCommand.modelDir) == false){
					logging.error("saving database failed");
				}
				else{
					logging.error("saving database successful");
				}

				keywordData.clear();
				pictureData.clear();
				
			}
			else if(prevCommand.command == ResonanzCommand::CMD_DO_OPTIMIZE){
				// stops computation if needed
				if(optimizer != nullptr){
				        optimizer->stopComputation();
					delete optimizer;
					optimizer = nullptr;
				}

				if(bayes_optimizer != nullptr){
					bayes_optimizer->stopSampler();
					delete bayes_optimizer;
					bayes_optimizer = nullptr;
				}

				// also saves database because preprocessing parameters may have changed
				if(engine_saveDatabase(prevCommand.modelDir) == false){
					logging.error("saving database failed");
				}
				else{
					logging.error("saving database successful");
				}

				// removes unnecessarily data structures from memory (measurements database) [no need to save it because it was not changed]
				keywordData.clear();
				pictureData.clear();
			}
			else if(prevCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
			        // stop playing sound
			        if(synth){
				  synth->pause();
				  synth->reset();
			        }
				
				keywordData.clear();
				pictureData.clear();
				keywordModels.clear();
				pictureModels.clear();

				if(prevCommand.audioFile.length() > 0){
				        logging.info("stop audio file playback");
					engine_stopAudioFile();
				}

				if(mcsamples.size() > 0)
					mcsamples.clear();

				// stops encoding if needed
				if(video != nullptr){
					auto t1 = std::chrono::system_clock::now().time_since_epoch();
					auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

					logging.info("stopping theora video encoding.");

					video->stopEncoding(t1ms - programStarted);
					delete video;
					video = nullptr;
					programStarted = 0;
				}
			}
			else if(prevCommand.command == ResonanzCommand::CMD_DO_MEASURE_PROGRAM){

			  // clears internal data structure
			  rawMeasuredSignals.clear();
		    
			  if(prevCommand.audioFile.length() > 0)
			    engine_stopAudioFile();
			}

			// state exit/entry actions:

			// checks if we want to have open graphics window and opens one if needed
			if(currentCommand.showScreen == true && prevCommand.showScreen == false){
				if(window != nullptr) SDL_DestroyWindow(window);
				
				SDL_DisplayMode mode;
				
				if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
				  SCREEN_WIDTH = mode.w;
				  SCREEN_HEIGHT = mode.h;
				}

				if(fullscreen){
				  window = SDL_CreateWindow(windowTitle.c_str(),
							    SDL_WINDOWPOS_CENTERED,
							    SDL_WINDOWPOS_CENTERED,
							    SCREEN_WIDTH, SCREEN_HEIGHT,
							    SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
				}
				else{
				  window = SDL_CreateWindow(windowTitle.c_str(),
							    SDL_WINDOWPOS_CENTERED,
							    SDL_WINDOWPOS_CENTERED,
							    (3*SCREEN_WIDTH)/4, (3*SCREEN_HEIGHT)/4,
							    SDL_WINDOW_SHOWN);
				}

				if(window != nullptr){
					SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
					if(font) TTF_CloseFont(font);
					double fontSize = 100.0*sqrt(((float)(SCREEN_WIDTH*SCREEN_HEIGHT))/(640.0*480.0));
					unsigned int fs = (unsigned int)fontSize;
					if(fs <= 0) fs = 10;
					
					font = 0;
					font = TTF_OpenFont(fontname.c_str(), fs);
				        
					
					SDL_Surface* icon = IMG_Load(iconFile.c_str());
					if(icon != nullptr){
						SDL_SetWindowIcon(window, icon);
						SDL_FreeSurface(icon);
					}

					SDL_Surface* surface = SDL_GetWindowSurface(window);
					SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
					SDL_RaiseWindow(window);
					// SDL_SetWindowGrab(window, SDL_TRUE);
					SDL_UpdateWindowSurface(window);
					SDL_RaiseWindow(window);
					// SDL_SetWindowGrab(window, SDL_FALSE);
				}

			}
			else if(currentCommand.showScreen == true && prevCommand.showScreen == true){
				// just empties current window with blank (black) screen
			  
			  	SDL_DisplayMode mode;
				
				if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
				  SCREEN_WIDTH = mode.w;
				  SCREEN_HEIGHT = mode.h;
				}
			  
				if(window == nullptr){
				        if(fullscreen){
				          window = SDL_CreateWindow(windowTitle.c_str(),
								    SDL_WINDOWPOS_CENTERED,
								    SDL_WINDOWPOS_CENTERED,
								    SCREEN_WIDTH, SCREEN_HEIGHT,
								    SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
					}
					else{
				          window = SDL_CreateWindow(windowTitle.c_str(),
								    SDL_WINDOWPOS_CENTERED,
								    SDL_WINDOWPOS_CENTERED,
								    (3*SCREEN_WIDTH)/4, (3*SCREEN_HEIGHT)/4,
								    SDL_WINDOW_SHOWN);
					}
				}

				if(window != nullptr){
					SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
					if(font) TTF_CloseFont(font);
					double fontSize = 100.0*sqrt(((float)(SCREEN_WIDTH*SCREEN_HEIGHT))/(640.0*480.0));
					unsigned int fs = (unsigned int)fontSize;
					if(fs <= 0) fs = 10;
					
					font = 0;
					font = TTF_OpenFont(fontname.c_str(), fs);

					SDL_Surface* icon = IMG_Load(iconFile.c_str());
					if(icon != nullptr){
						SDL_SetWindowIcon(window, icon);
						SDL_FreeSurface(icon);
					}

					SDL_Surface* surface = SDL_GetWindowSurface(window);
					SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
					SDL_RaiseWindow(window);
					// SDL_SetWindowGrab(window, SDL_TRUE);
					SDL_UpdateWindowSurface(window);
					SDL_RaiseWindow(window);
					// SDL_SetWindowGrab(window, SDL_FALSE);
				}

			}
			else if(currentCommand.showScreen == false){
				if(window != nullptr) SDL_DestroyWindow(window);
				window = nullptr;
			}

			// state entry actions:

			if(currentCommand.command == ResonanzCommand::CMD_DO_MEASURE || currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
				std::lock_guard<std::mutex> lock(eeg_mutex);
				if(eeg->connectionOk() == false){
					logging.warn("eeg: no connection to eeg hardware => aborting measure/execute command");
					cmdDoNothing(false);
					continue;
				}
			}

			// (re)loads media resources (pictures, keywords) if we want to do stimulation
			if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM || currentCommand.command == ResonanzCommand::CMD_DO_MEASURE ||
					currentCommand.command == ResonanzCommand::CMD_DO_OPTIMIZE || currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
				engine_setStatus("resonanz-engine: loading media files..");

				bool loadData = (currentCommand.command != ResonanzCommand::CMD_DO_OPTIMIZE);

				if(engine_loadMedia(currentCommand.pictureDir, currentCommand.keywordsFile, loadData) == false){
					logging.error("loading media files failed");
				}
				else{
				    char buffer[80];
					snprintf(buffer, 80, "loading media files successful (%d keywords, %d pics)",
							(int)keywords.size(), (int)pictures.size());
					logging.info(buffer);
				}
			}
			

			// (re)-setups and initializes data structures used for measurements
			if(currentCommand.command == ResonanzCommand::CMD_DO_MEASURE ||
					currentCommand.command == ResonanzCommand::CMD_DO_OPTIMIZE ||
					currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE)
			{
				engine_setStatus("resonanz-engine: loading database..");

				if(engine_loadDatabase(currentCommand.modelDir) == false)
					logging.error("loading database files failed");
				else
					logging.info("loading database files successful");
			}

			if(currentCommand.command == ResonanzCommand::CMD_DO_OPTIMIZE){
				engine_setStatus("resonanz-engine: initializing prediction model optimization..");
				currentPictureModel = 0;
				currentKeywordModel = 0;
				soundModelCalculated = false;

				if(this->use_bayesian_nnetwork)
					logging.info("model optimization uses BAYESIAN UNCERTAINTY estimation through sampling");

				// checks there is enough data to do meaningful optimization
				bool aborted = false;

				for(unsigned int i=0;i<pictureData.size() && !aborted;i++){
					if(pictureData[i].size(0) < 10){
						engine_setStatus("resonanz-engine: less than 10 data points per picture/keyword => aborting optimization");
						logging.warn("aborting model optimization command because of too little data (less than 10 samples per case)");
						cmdDoNothing(false);
						aborted = true;
						break;
					}
				}

				for(unsigned int i=0;i<keywordData.size() && !aborted;i++){
					if(keywordData[i].size(0) < 10){
						engine_setStatus("resonanz-engine: less than 10 data points per picture/keyword => aborting optimization");
						logging.warn("aborting model optimization command because of too little data (less than 10 samples per case)");
						cmdDoNothing(false);
						aborted = true;
						break;
					}
				}
				
				if(synth){
				  if(synthData.size(0) < 10){
				    engine_setStatus("resonanz-engine: less than 10 data points per picture/keyword => aborting optimization");
				    logging.warn("aborting model optimization command because of too little data (less than 10 samples per case)");
				    cmdDoNothing(false);
				    aborted = true;
				    break;
				  }
				}

				if(aborted)
					continue; // do not start executing any commands [recheck command input buffer and move back to do nothing command]
				
				optimizeETA.start(0.0f, 1.0f);
			}

			
			if(currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
				try{
				        engine_setStatus("resonanz-engine: loading prediction model..");

					if(engine_loadModels(currentCommand.modelDir) == false && dataRBFmodel == false){
						logging.error("Couldn't load models from model dir: " + currentCommand.modelDir);
						this->cmdStopCommand();
						continue; // aborts initializing execute command
					}

					logging.info("Converting program (targets) to internal format..");

					// convert input command parameters into generic targets that are used to select target values
					std::vector<std::string> names;
					eeg->getSignalNames(names);

					// inits program values into "no program" values
					program.resize(names.size());
					for(auto& p : program){
						p.resize(currentCommand.programValues[0].size());
						for(unsigned int i=0;i<p.size();i++)
							p[i] = 0.5f;
					}

					programVar.resize(names.size());
					for(auto& p : programVar){
						p.resize(currentCommand.programValues[0].size());
						for(unsigned int i=0;i<p.size();i++)
							p[i] = 1000000.0f; // 1.000.000 very large value (near infinite) => can take any value
					}

					for(unsigned int j=0;j<currentCommand.signalName.size();j++){
						for(unsigned int n=0;n<names.size();n++){
							if(names[n] == currentCommand.signalName[j]){ // finds a matching signal in a command
								for(unsigned int i=0;i<program[n].size();i++){
									if(currentCommand.programValues[j][i] >= 0.0f){
										program[n][i] = currentCommand.programValues[j][i];
										programVar[n][i] = 1.0f; // "normal" variance
									}
								}
							}
						}
					}

					logging.info("Converting program (targets) to internal format.. DONE.");


					// initializes blind monte carlo data structures
					if(currentCommand.blindMonteCarlo){
					        logging.info("Blind Monte Carlo mode activated/initialization...");
						mcsamples.clear();

						for(unsigned int i=0;i<MONTE_CARLO_SIZE;i++){
						        math::vertex<> u(names.size());
							for(unsigned int j=0;j<u.size();j++)
							        u[j] = rng.uniform(); // [0,1] valued signals sampled from [0,1]^D
							mcsamples.push_back(u);
						}
					}

					
					if(currentCommand.audioFile.length() > 0){
					        logging.info("play audio file");
						engine_playAudioFile(currentCommand.audioFile);
					}

					// starts measuring time for the execution of the program

					auto t0 = std::chrono::system_clock::now().time_since_epoch();
					auto t0ms = std::chrono::duration_cast<std::chrono::milliseconds>(t0).count();
					programStarted = t0ms;
					lastProgramSecond = -1;
					
					// RMS performance error calculation
					programRMS = 0.0f;
					programRMS_N = 0;

					logging.info("Started executing neurostim program..");

				}
				catch(std::exception& e){

				}
			}
			if(currentCommand.command == ResonanzCommand::CMD_DO_MEASURE_PROGRAM){
				// checks command signal names maps to some eeg signals
				std::vector<std::string> names;
				eeg->getSignalNames(names);

				unsigned int matches = 0;
				for(auto& n : names){
					for(auto& m : currentCommand.signalName){
						if(n == m){ // string comparion
							matches++;
						}
					}
				}

				if(matches == 0){
					logging.warn("resonanz-engine: measure program signal names don't match to device signals");

					this->cmdDoNothing(false); // abort
					continue;
				}

				rawMeasuredSignals.resize(names.size()); // setups data structure for measurements

				// invalidates old program
				this->invalidateMeasuredProgram();

				// starts measuring time for the execution of the program

				auto t0 = std::chrono::system_clock::now().time_since_epoch();
				auto t0ms = std::chrono::duration_cast<std::chrono::milliseconds>(t0).count();
				programStarted = t0ms;
				lastProgramSecond = -1;
				eegConnectionDownTime = 0;

				// currently just plays audio and shows blank screen
				if(currentCommand.audioFile.length() > 0)
					engine_playAudioFile(currentCommand.audioFile);

				// => ready to measure
			}
			
			
			if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM || currentCommand.command == ResonanzCommand::CMD_DO_MEASURE ||
			   currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
			        engine_setStatus("resonanz-engine: starting sound synthesis..");

				if(currentCommand.audioFile.length() <= 0){
				  if(synth){
				    if(synth->play() == false){
				      logging.error("starting sound synthesis failed");
				    }
				    else{
				      logging.info("starting sound synthesis..OK");
				    }
				  }
				}
			}


			if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM || currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
			  if(currentCommand.saveVideo){
			    logging.info("Starting video encoder (theora)..");
			    
			    // starts video encoder
			    video = new SDLTheora(0.50f); // 50% quality
			    
			    if(video->startEncoding("neurostim.ogv", SCREEN_WIDTH, SCREEN_HEIGHT) == false)
			      logging.error("starting theora video encoder failed");
			    else
			      logging.info("started theora video encoding");
			  }
			  else{
			    // do not save video
			    video = nullptr;
			  }
			}


			if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM){
			  auto t0 = std::chrono::system_clock::now().time_since_epoch();
			  auto t0ms = std::chrono::duration_cast<std::chrono::milliseconds>(t0).count();
			  programStarted = t0ms;
			  lastProgramSecond = -1;

			  if(currentCommand.audioFile.length() > 0){
			    logging.info("play audio file");
			    engine_playAudioFile(currentCommand.audioFile);
			  }
			}
			
		}
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		// executes current command
		
		if(currentCommand.command == ResonanzCommand::CMD_DO_NOTHING){
			engine_setStatus("resonanz-engine: sleeping..");

			engine_pollEvents(); // polls for events
			engine_updateScreen(); // always updates window if it exists
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM){
			engine_setStatus("resonanz-engine: showing random examples..");

			engine_stopHibernation();

			if(pictures.size() > 0){
				if(keywords.size() > 0){
				        auto& key = currentKey;
					auto& pic = currentPic;
					
				        if(tick - latestKeyPicChangeTick > SHOWTIME_TICKS){
					  key = rng.rand() % keywords.size();
					  pic = rng.rand() % pictures.size();
					  
					  latestKeyPicChangeTick = tick;
					}
					
					std::vector<float> sndparams;

					if(synth != NULL){
					  sndparams.resize(synth->getNumberOfParameters());
					  for(unsigned int i=0;i<sndparams.size();i++)
					    sndparams[i] = rng.uniform().c[0];
					}

					if(engine_showScreen(keywords[key], pic, sndparams) == false)
						logging.warn("random stimulus: engine_showScreen() failed.");
				}
				else{
				        auto& pic = currentPic;
					
				        if(tick - latestKeyPicChangeTick > SHOWTIME_TICKS){
					  pic = rng.rand() % pictures.size();
					  
					  latestKeyPicChangeTick = tick;
					}
					
					std::vector<float> sndparams;

					if(synth != NULL){
					  sndparams.resize(synth->getNumberOfParameters());
					  for(unsigned int i=0;i<sndparams.size();i++)
					    sndparams[i] = rng.uniform().c[0];
					}

					if(engine_showScreen(" ", pic, sndparams) == false)
						logging.warn("random stimulus: engine_showScreen() failed.");
				}
			}

			engine_pollEvents(); // polls for events
			engine_updateScreen(); // always updates window if it exists
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_MEASURE){
			engine_setStatus("resonanz-engine: measuring eeg-responses..");

			if(eeg->connectionOk() == false){
			        eegConnectionDownTime = TICK_MS*(tick - eegLastTickConnectionOk);
				
				if(eegConnectionDownTime >= 2000){
				  logging.info("measure command: eeg connection failed => aborting measurements");
				  cmdDoNothing(false); // new command: stops and starts idling
				}
				  
				engine_pollEvents(); // polls for events
				engine_updateScreen(); // always updates window if it exists

				continue;
			}
			else{
			  eegConnectionDownTime = 0;
			  eegLastTickConnectionOk = tick;
			}
			

			engine_stopHibernation();

			if(keywords.size() > 0 && pictures.size() > 0){
				unsigned int key = rng.rand() % keywords.size();
				unsigned int pic = rng.rand() % pictures.size();

				std::vector<float> eegBefore;
				std::vector<float> eegAfter;
				
				std::vector<float> synthBefore;
				std::vector<float> synthCurrent;
				
				if(synth){
				  synth->getParameters(synthBefore);
				  
				  synthCurrent.resize(synth->getNumberOfParameters());
				  
				  if(rng.uniform() < 0.20f){
				    // total random sound
				    for(unsigned int i=0;i<synthCurrent.size();i++)
				      synthCurrent[i] = rng.uniform().c[0];
				  }
				  else{
				    // generates something similar (adds random noise to current parameters)
				    for(unsigned int i=0;i<synthCurrent.size();i++){
				      synthCurrent[i] = synthBefore[i] + rng.normal().c[0]*0.10f;
				      if(synthCurrent[i] <= 0.0f) synthCurrent[i] = 0.0f;
				      else if(synthCurrent[i] >= 1.0f) synthCurrent[i] = 1.0f;
				    }
				  }
				  
				}
				
				eeg->data(eegBefore);

				engine_showScreen(keywords[key], pic, synthCurrent);
				engine_updateScreen(); // always updates window if it exists
				engine_sleep(MEASUREMODE_DELAY_MS);
				
				eeg->data(eegAfter);

				engine_pollEvents();

				if(engine_storeMeasurement(pic, key, eegBefore, eegAfter,
							   synthBefore, synthCurrent) == false)
				  logging.error("Store measurement failed");
			}
			else if(pictures.size() > 0){
				unsigned int pic = rng.rand() % pictures.size();

				std::vector<float> eegBefore;
				std::vector<float> eegAfter;
				
				std::vector<float> synthBefore;
				std::vector<float> synthCurrent;
				
				if(synth){
				  synth->getParameters(synthBefore);
				  
				  synthCurrent.resize(synth->getNumberOfParameters());
				  
				  if(rng.uniform() < 0.20f){
				    // total random sound
				    for(unsigned int i=0;i<synthCurrent.size();i++)
				      synthCurrent[i] = rng.uniform().c[0];
				  }
				  else{
				    // generates something similar (adds random noise to current parameters)
				    for(unsigned int i=0;i<synthCurrent.size();i++){
				      synthCurrent[i] = synthBefore[i] + rng.normal().c[0]*0.10f;
				      if(synthCurrent[i] <= 0.0f) synthCurrent[i] = 0.0f;
				      else if(synthCurrent[i] >= 1.0f) synthCurrent[i] = 1.0f;
				    }
				  }
				}
				
				eeg->data(eegBefore);
				
				engine_showScreen(" ", pic, synthCurrent);
				engine_updateScreen(); // always updates window if it exists
				engine_sleep(MEASUREMODE_DELAY_MS);
				
				eeg->data(eegAfter);

				engine_pollEvents();

				if(engine_storeMeasurement(pic, 0, eegBefore, eegAfter,
							   synthBefore, synthCurrent) == false)
				  logging.error("store measurement failed");

			}
			else{
				engine_pollEvents(); // polls for events
				engine_updateScreen(); // always updates window if it exists
			}
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_OPTIMIZE){
		        const float percentage =
			  (currentPictureModel + currentKeywordModel + (soundModelCalculated == true))/((float)(pictureData.size()+keywordData.size()+1));

			optimizeETA.update(percentage);

			{
				float eta = optimizeETA.estimate();
				eta = eta / 60.0f; // ETA in minutes

				char buffer[160];
				snprintf(buffer, 160, "resonanz-engine: optimizing prediction model (%.2f%%) [ETA %.1f min]..",
						100.0f*percentage, eta);

				engine_setStatus(buffer);
			}

			engine_stopHibernation();

		        if(engine_optimizeModels(currentPictureModel, currentKeywordModel, soundModelCalculated) == false)
				logging.warn("model optimization failure");

		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
		  logging.info("resonanz-engine: executing program..");
		  engine_setStatus("resonanz-engine: executing program..");
		  
		  engine_stopHibernation();
		  
		  auto t1 = std::chrono::system_clock::now().time_since_epoch();
		  auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
		  
		  long long currentSecond = (long long)
		    (programHz*(t1ms - programStarted)/1000.0f); // gets current second for the program value
			
		  
		  if(loopMode){
		    
		    if(currentSecond/programHz >= program[0].size()){ // => restarts program
		      currentSecond = 0;
		      lastProgramSecond = -1;
		      
		      auto t1 = std::chrono::system_clock::now().time_since_epoch();
		      auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
		      
		      programStarted = (long long)t1ms;
		    }
		  }
		  
		  if(currentSecond > lastProgramSecond && lastProgramSecond >= 0){
		    eeg->data(eegCurrent);

		    logging.info("Calculating RMS error");
		    
		    // calculates RMS error
		    std::vector<float> current;
		    std::vector<float> target;
		    std::vector<float> eegTargetVariance;
		    
		    target.resize(program.size());
		    eegTargetVariance.resize(program.size());
		    
		    for(unsigned int i=0;i<program.size();i++){
		      // what was our target BEFORE this time tick [did we move to target state]
		      target[i] = program[i][lastProgramSecond/programHz];
		      eegTargetVariance[i] = programVar[i][lastProgramSecond/programHz];
		    }
		    
		    eeg->data(current);
		    
		    if(target.size() == current.size()){
		      float rms = 0.0f;
		      for(unsigned int i=0;i<target.size();i++){
			rms += (current[i] - target[i])*(current[i] - target[i])/eegTargetVariance[i];
		      }
		      
		      rms = sqrt(rms);
		      
		      // adds current rms to the global RMS
		      programRMS += rms;
		      programRMS_N++;
		      
		      {
			char buffer[256];
			snprintf(buffer, 256, "Program current RMS error: %.2f (average RMS error: %.2f)",
				 rms, programRMS/programRMS_N);
			logging.info(buffer);
		      }					
		    }
		  }
		  else if(currentSecond > lastProgramSecond && lastProgramSecond < 0){
		    eeg->data(eegCurrent);
		  }
		  
		  lastProgramSecond = currentSecond;
		  
		  {
		    char buffer[80];
		    snprintf(buffer, 80, "Executing program (pseudo)second: %d/%d",
			     (unsigned int)(currentSecond/programHz), program[0].size());
		    logging.info(buffer);
		  }
		  
		  
		  if(currentSecond/programHz < (signed)program[0].size()){
		    logging.info("Executing program: calculating current targets");
		    
		    // executes program
		    std::vector<float> eegTarget;
		    std::vector<float> eegTargetVariance;
		    
		    eegTarget.resize(eegCurrent.size());
		    eegTargetVariance.resize(eegCurrent.size());
			  
		    for(unsigned int i=0;i<eegTarget.size();i++){
		      eegTarget[i] = program[i][currentSecond/programHz];
		      eegTargetVariance[i] = programVar[i][currentSecond/programHz];
		    }
		    
		    // shows picture/keyword which model predicts to give closest match to target
		    // minimize(picture) ||f(picture,eegCurrent) - eegTarget||/eegTargetVariance
		    
		    const float timedelta = 1.0f/programHz; // current delta between pictures [in seconds]

		    
		    if(currentCommand.blindMonteCarlo == false)
		      engine_executeProgram(eegCurrent, eegTarget, eegTargetVariance, timedelta);
		    else
		      engine_executeProgramMonteCarlo(eegTarget, eegTargetVariance, timedelta);
		  }
		  else{
		    
		    // program has run to the end => stop
		    logging.info("Executing the given program has stopped [program stop time].");
		    
		    if(video){
		      auto t1 = std::chrono::system_clock::now().time_since_epoch();
		      auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
		      
		      logging.info("stopping theora video encoding.");
		      
		      video->stopEncoding(t1ms - programStarted);
		      delete video;
		      video = nullptr;
		    }
		    
		    cmdStopCommand();
		  }
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_MEASURE_PROGRAM){
		  engine_setStatus("resonanz-engine: measuring program..");
		  
		  engine_stopHibernation();
		  
		  auto t1 = std::chrono::system_clock::now().time_since_epoch();
		  auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
		  
		  long long currentSecond = (long long)
		    (programHz*(t1ms - programStarted)/1000.0f); // gets current second for the program value
		  
		  if(currentSecond <= lastProgramSecond)
		    continue; // nothing to do

		  // measures new measurements
		  for(;lastProgramSecond <= currentSecond; lastProgramSecond++){
		    // measurement program continues: just measures values and do nothing
		    // as the background thread currently handles playing of the music
		    // LATER: do video decoding and showing..
		    
		    std::vector<float> values(eeg->getNumberOfSignals());
		    eeg->data(values);
		    
		    for(unsigned int i=0;i<rawMeasuredSignals.size();i++)
		      rawMeasuredSignals[i].push_back(values[i]);
		  }
		  
		  
		  if(currentSecond < currentCommand.programLengthTicks){
		    engine_updateScreen();
		    engine_pollEvents();
		  }
		  else{
		    // stops measuring program

		    // transforms raw signals into measuredProgram values
		    
		    std::lock_guard<std::mutex> lock(measure_program_mutex);
		    
		    std::vector<std::string> names;
		    eeg->getSignalNames(names);
		    
		    measuredProgram.resize(currentCommand.signalName.size());
		    
		    for(unsigned int i=0;i<measuredProgram.size();i++){
		      measuredProgram[i].resize(currentCommand.programLengthTicks);
		      for(auto& m : measuredProgram[i])
			m = -1.0f;
		    }
		    
		    for(unsigned int j=0;j<currentCommand.signalName.size();j++){
		      for(unsigned int n=0;n<names.size();n++){
			if(names[n] == currentCommand.signalName[j]){ // finds a matching signal in a command
			  unsigned int MIN = measuredProgram[j].size();
			  if(rawMeasuredSignals[n].size() < MIN*programHz)
			    MIN = rawMeasuredSignals[n].size()/programHz;
			  
			  for(unsigned int i=0;i<MIN;i++){
			    
			    auto mean = 0.0f;
			    auto N = 0.0f;
			    for(unsigned int k=0;k<programHz;k++){
			      if(rawMeasuredSignals[n][i*programHz + k] >= 0.0f){
				mean += rawMeasuredSignals[n][i*programHz + k];
				N++;
			      }
			    }
			    
			    if(N > 0.0f)
			      measuredProgram[j][i] = mean / N;
			    else
			      measuredProgram[j][i] = 0.5f;
			    
			  }
			  
			}
		      }
		    }
		    
		    
		    cmdStopCommand();
		  }
		}
		
		engine_pollEvents();

		if(keypress()){
		  if(currentCommand.command != ResonanzCommand::CMD_DO_NOTHING &&
		     currentCommand.command != ResonanzCommand::CMD_DO_MEASURE_PROGRAM)
		  {
		    logging.info("Received keypress: stopping command..");
		    cmdStopCommand();
		  }
		}

		// monitors current eeg values and logs them into log file
		{
			std::lock_guard<std::mutex> lock(eeg_mutex); // mutex might change below use otherwise..

			if(eeg->connectionOk() == false){
				std::string line = "eeg ";
				line += eeg->getDataSourceName();
				line += " : no connection to hardware";
				logging.info(line);
			}
			else{
				std::string line = "eeg ";
				line += eeg->getDataSourceName();
				line + " :";

				std::vector<float> x;
				eeg->data(x);

				for(unsigned int i=0;i<x.size();i++){
					char buffer[80];
					snprintf(buffer, 80, " %.2f", x[i]);
					line += buffer;
				}

				logging.info(line);
			}
		}

	}

	if(window != nullptr)
		SDL_DestroyWindow(window);

	{
		std::lock_guard<std::mutex> lock(eeg_mutex);
		if(eeg) delete eeg;
		eeg = nullptr;
		eegDeviceType = ResonanzEngine::RE_EEG_NO_DEVICE;
	}

	if(nn != nullptr){
		delete nn;
		nn = nullptr;
	}
	
	if(nnsynth != nullptr){
	        delete nnsynth;
		nnsynth = nullptr;
	}

	if(bnn != nullptr){
		delete bnn;
		bnn = nullptr;
	}

	engine_SDL_deinit();

}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// loads prediction models for program execution, returns false in case of failure
bool ResonanzEngine::engine_loadModels(const std::string& modelDir)
{
	if(pictures.size() <= 0)
		return false; // no loaded pictures       	

	pictureModels.resize(pictures.size());
	
	whiteice::linear_ETA<float> loadtimeETA;
	loadtimeETA.start(0.0f, 1.0f);

	unsigned int pictureModelsLoaded = 0;
	unsigned int keywordModelsLoaded = 0;

	// #pragma omp parallel for
	for(unsigned int i=0;i<pictureModels.size();i++){
		std::string filename = calculateHashName(pictures[i] + eeg->getDataSourceName()) + ".model";
		filename = modelDir + "/" + filename;

		if(pictureModels[i].load(filename) == false){
			logging.error("Loading picture model file failed: " + filename);
			continue;
		}
		
		
		pictureModels[i].downsample(100); // keeps only 100 random models

		pictureModelsLoaded++;
		
		{
		  char buffer[100];
		  const float percentage = ((float)i)/((float)(keywords.size()+pictures.size()+1));
		  
		  loadtimeETA.update(percentage);

		  snprintf(buffer, 100, "resonanz-engine: loading prediction model (%.1f%%) [ETA %.2f mins]..", 
			   100.0f*percentage, loadtimeETA.estimate()/60.0f);

		  logging.info(buffer);
		  engine_setStatus(buffer);
		}

	}

	keywordModels.resize(keywords.size());

	// #pragma omp parallel for
	for(unsigned int i=0;i<keywordModels.size();i++){
		std::string filename = calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".model";
		filename = modelDir + "/" + filename;

		if(keywordModels[i].load(filename) == false){
			logging.error("Loading keyword model file failed: " + filename);
			continue;
		}
		
		keywordModels[i].downsample(100); // keeps only 100 random samples

		keywordModelsLoaded++;
		
		{
		  char buffer[100];
		  const float percentage = 
		    ((float)(i + pictures.size()))/((float)(keywords.size()+pictures.size()+1));
		  
		  loadtimeETA.update(percentage);
		  
		  snprintf(buffer, 100, "resonanz-engine: loading prediction model (%.1f%%) [ETA %.2f mins]..", 
			   100.0f*percentage, loadtimeETA.estimate()/60.0f);

		  logging.info(buffer);
		  engine_setStatus(buffer);
		}
	}
	
	
	if(synth){
	  std::string filename = calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".model";
	  filename = modelDir + "/" + filename;

	  if(synthModel.load(filename) == false){
	    logging.error("Loading synth model file failed: " + filename);
	  }
	  else{
	    char buffer[256];
	    snprintf(buffer, 256, "loading synth model success: %d - %d",
		     synthModel.inputSize(), synthModel.outputSize());
	    logging.info(buffer);
	  }
		
	  synthModel.downsample(100); // keeps only 100 random models
	}

	// returns true if could load at least one model for pictures
	return (pictureModelsLoaded > 0);
}


// shows picture/keyword which model predicts to give closest match to target
// minimize(picture) ||f(picture,eegCurrent) - eegTarget||/eegTargetVariance
bool ResonanzEngine::engine_executeProgram(const std::vector<float>& eegCurrent,
					   const std::vector<float>& eegTarget, 
					   const std::vector<float>& eegTargetVariance,
					   float timestep_)
{
	const unsigned int NUM_TOPRESULTS = 1;
	std::multimap<float, int> bestKeyword;
	std::multimap<float, int> bestPicture;
	
	std::vector<float> soundParameters;

	math::vertex<> target(eegTarget.size());
	math::vertex<> targetVariance(eegTargetVariance.size());
	const whiteice::math::blas_real<float> timestep = timestep_;

	for(unsigned int i=0;i<target.size();i++){
		target[i] = eegTarget[i];
		targetVariance[i] = eegTargetVariance[i];
	}

	std::vector< std::pair<float, int> > results(keywordData.size());
	std::vector< float > model_error_ratio(keywordData.size());

	logging.info("engine_executeProgram() calculate keywords");

#pragma omp parallel for schedule(dynamic)	
	for(unsigned int index=0;index<keywordData.size();index++){

		math::vertex<> x(eegCurrent.size());
		for(unsigned int i=0;i<x.size();i++)
			x[i] = eegCurrent[i];

		auto original = x;

		if(keywordData[index].preprocess(0, x) == false){
			logging.warn("skipping bad keyword prediction model");
			continue;
		}

		math::vertex<> m;
		math::matrix<> cov;

		if(dataRBFmodel){
			engine_estimateNN(x, keywordData[index], m , cov);
		}
		else{
			whiteice::bayesian_nnetwork<>& model = keywordModels[index];

			if(model.inputSize() != eegCurrent.size() || model.outputSize() != eegTarget.size()){
				logging.warn("skipping bad keyword prediction model");
				continue; // bad model/data => ignore
			}

			if(model.calculate(x, m, cov, 1, 0) == false){
				logging.warn("skipping bad keyword prediction model");
				continue;
			}
		}


		if(keywordData[index].invpreprocess(1, m) == false){
			logging.warn("skipping bad keyword prediction model");
			continue;
		}

		m *= timestep; // corrects delta to given timelength
		cov *= timestep*timestep;

		// now we have prediction x to the response to the given keyword
		// calculates error (weighted distance to the target state)

		auto delta = target - (original + m);
		auto stdev = m;
		
		for(unsigned int i=0;i<stdev.size();i++){
		  stdev[i] = math::sqrt(math::abs(cov(i,i)));
		}
		
		// calculates average stdev/delta ratio
		auto ratio = stdev.norm()/m.norm();
		model_error_ratio[index] = ratio.c[0];
		
		for(unsigned int i=0;i<delta.size();i++){
		  delta[i] = math::abs(delta[i]) + 0.5f*stdev[i];
		  delta[i] /= targetVariance[i];
		}

		auto error = delta.norm();

		std::pair<float, int> p;
		p.first = error.c[0];
		p.second = index;

		results[index] = p;

		// engine_pollEvents(); // polls for incoming events in case there are lots of models
	}
	
	
	// estimates quality of results
	if(model_error_ratio.size() > 0)
	{
	  float mean_ratio = 0.0f;
	  for(auto& r : model_error_ratio)
	    mean_ratio += r;
	  mean_ratio /= model_error_ratio.size();
	  
	  if(mean_ratio > 1.0f){
	    char buffer[80];
	    snprintf(buffer, 80, "Optimizing program: KEYWORD PREDICTOR ERROR LARGER THAN OUTPUT (%.2f larger)", mean_ratio);
	    logging.warn(buffer);	    
	  }
	}
	
	
	// selects the best result

	for(auto& p : results)
		bestKeyword.insert(p);

	while(bestKeyword.size() > NUM_TOPRESULTS){
		auto i = bestKeyword.end(); i--;
		bestKeyword.erase(i); // removes the largest element
	}

	engine_pollEvents();


	results.resize(pictureData.size());
	model_error_ratio.resize(pictureData.size());

	logging.info("engine_executeProgram(): calculate pictures");

#pragma omp parallel for schedule(dynamic)	
	for(unsigned int index=0;index<pictureData.size();index++){

		math::vertex<> x(eegCurrent.size());
		for(unsigned int i=0;i<x.size();i++)
			x[i] = eegCurrent[i];

		auto original = x;

		if(pictureData[index].preprocess(0, x) == false){
			logging.warn("skipping bad picture prediction model");
			continue;
		}

		math::vertex<> m;
		math::matrix<> cov;

		if(dataRBFmodel){
			engine_estimateNN(x, pictureData[index], m , cov);
		}
		else{
			whiteice::bayesian_nnetwork<>& model = pictureModels[index];

			if(model.inputSize() != eegCurrent.size() || model.outputSize() != eegTarget.size()){
				logging.warn("skipping bad picture prediction model");
				continue; // bad model/data => ignore
			}

			if(model.calculate(x, m, cov, 1, 0) == false){
				logging.warn("skipping bad picture prediction model");
				continue;
			}

		}

		if(pictureData[index].invpreprocess(1, m) == false){
			logging.warn("skipping bad picture prediction model");
			continue;
		}

		m *= timestep; // corrects delta to given timelength
		cov *= timestep*timestep;

		// now we have prediction x to the response to the given keyword
		// calculates error (weighted distance to the target state)
		
		auto delta = target - (original + m);
		auto stdev = m;
		
		for(unsigned int i=0;i<stdev.size();i++){
		  stdev[i] = math::sqrt(math::abs(cov(i,i)));
		}

		// calculates average stdev/delta ratio
		auto ratio = stdev.norm()/m.norm();
		model_error_ratio[index] = ratio.c[0];
		
		for(unsigned int i=0;i<delta.size();i++){
		  delta[i] = math::abs(delta[i]) + 0.5f*stdev[i];
		  delta[i] /= targetVariance[i];
		}

		auto error = delta.norm();

		std::pair<float, int> p;
		p.first = error.c[0];
		p.second = index;

		results[index] = p;
		
		// engine_pollEvents(); // polls for incoming events in case there are lots of models
	}
	
	
	// estimates quality of results
	if(model_error_ratio.size() > 0)
	{
	  float mean_ratio = 0.0f;
	  for(auto& r : model_error_ratio)
	    mean_ratio += r;
	  mean_ratio /= model_error_ratio.size();
	  
	  if(mean_ratio > 1.0f){
	    char buffer[80];
	    snprintf(buffer, 80, "Optimizing program: PICTURE PREDICTOR ERROR LARGER THAN OUTPUT (%.2f larger)", mean_ratio);
	    logging.warn(buffer);	    
	  }
	}
	
	
	for(auto& p : results)
		bestPicture.insert(p);

	while(bestPicture.size() > NUM_TOPRESULTS){
		auto i = bestPicture.end(); i--;
		bestPicture.erase(i); // removes the largest element
	}

	engine_pollEvents(); // polls for incoming events in case there are lots of models
	
	
	if(synth){
	  logging.info("engine_executeProgram(): calculate synth model");
	  
	  // initial sound parameters are random
	  soundParameters.resize(synth->getNumberOfParameters());
	  for(unsigned int i=0;i<soundParameters.size();i++)
	    soundParameters[i] = rng.uniform().c[0];
	  
	  math::vertex<> input;
	  input.resize(synthModel.inputSize());
	  input.zero();
	  
	  std::vector<float> synthBefore;
	  std::vector<float> synthTest;
	  
	  synth->getParameters(synthBefore);
	  synthTest.resize(synthBefore.size());

	  // nn(synthNow, synthProposed, currentEEG) = dEEG/dt
	  if(2*synthBefore.size() + eegCurrent.size() != synthModel.inputSize()){
	    char buffer[256];
	    snprintf(buffer, 256, "engine_executeProgram(): synth model input parameters (dimension) mismatch! (%d + %d != %d)\n",
		     synthBefore.size(), eegCurrent.size(), synthModel.inputSize());
	    logging.fatal(buffer);
	  }
	  
	  for(unsigned int i=0;i<synthBefore.size();i++){
	    input[i] = synthBefore[i];
	  }
	  
	  for(unsigned int i=0;i<eegCurrent.size();i++){
	    input[2*synthBefore.size() + i] = eegCurrent[i];
	  }
	  
	  math::vertex<> original(eegCurrent.size());
	  
	  for(unsigned int i=0;i<original.size();i++)
	    original[i] = eegCurrent[i];
	  
	  std::vector< std::pair<float, std::vector<float> > > errors;
	  errors.resize(SYNTH_NUM_GENERATED_PARAMS);
	  
	  model_error_ratio.resize(SYNTH_NUM_GENERATED_PARAMS);
	  
	  // generates synth parameters randomly and selects parameter
	  // with smallest predicted error to target state

	  logging.info("engine_executeProgram(): parallel synth model search start..");

#pragma omp parallel for schedule(dynamic)
	  for(unsigned int param=0;param<SYNTH_NUM_GENERATED_PARAMS;param++){
	    
	    if(rng.uniform() < 0.10f)
	    {
	      // generates random parameters [random search]
	      for(unsigned int i=0;i<synthTest.size();i++)
		synthTest[i] = rng.uniform().c[0];
	    }
	    else
	    {
	      // or: adds gaussian noise to current parameters 
	      //     [random jumps around neighbourhood]
	      for(unsigned int i=0;i<synthTest.size();i++){
		synthTest[i] = synthBefore[i] + rng.normal().c[0]*0.10f;
		if(synthTest[i] <= 0.0f) synthTest[i] = 0.0f;
		else if(synthTest[i] >= 1.0f) synthTest[i] = 1.0f;
	      }
	    }

	    // copies parameters to input vector
	    for(unsigned int i=0;i<synthTest.size();i++){
	      input[synthBefore.size()+i] = synthTest[i];
	    }
	    
	    // calculates approximated response
	    auto x = input;
	    
	    if(synthData.preprocess(0, x) == false){
	      logging.warn("skipping bad synth prediction");
	      continue;
	    }

	    math::vertex<> m;
	    math::matrix<> cov;
	    
	    if(dataRBFmodel){
	      // engine_estimateNN(x, synthData, m , cov);
	      // NOT SUPPORTED YET...
	      
	      // now change high variance output
	      m = x;
	      cov.resize(x.size(), x.size());
	      cov.identity(); 
	    }
	    else{
	      auto& model = synthModel;
	      
	      if(model.inputSize() != x.size() || model.outputSize() != eegTarget.size()){
		logging.warn("skipping bad synth prediction model");
		continue; // bad model/data => ignore
	      }
	      
	      if(model.calculate(x, m, cov, 1, 0) == false){
		logging.warn("skipping bad synth prediction model");
		continue;
	      }
	    }

	    if(synthData.invpreprocess(1, m) == false){
	      logging.warn("skipping bad synth prediction model");
	      continue;
	    }

	    m *= timestep; // corrects delta to given timelength
	    cov *= timestep*timestep;

	    // now we have prediction m to the response to the given keyword
	    // calculates error (weighted distance to the target state)
	    
	    auto delta = target - (original + m);
	    auto stdev = m;

	    for(unsigned int i=0;i<stdev.size();i++){
	      stdev[i] = math::sqrt(math::abs(cov(i,i)));
	    }

	    // calculates average stdev/delta ratio
	    auto ratio = stdev.norm()/m.norm();
	    model_error_ratio[param] = ratio.c[0];
	    
	    for(unsigned int i=0;i<delta.size();i++){
	      delta[i] = math::abs(delta[i]) + 0.5f*stdev[i];
	      delta[i] /= targetVariance[i];
	    }

	    auto error = delta.norm();
	    
	    std::pair<float, std::vector<float> > p;
	    p.first = error.c[0];
	    p.second = synthTest;
	    
	    errors[param] = p;

	  }

	  logging.info("engine_executeProgram(): parallel synth model search start.. DONE");
	  
	  
	  // estimates quality of results
	  {
	    float mean_ratio = 0.0f;
	    for(auto& r : model_error_ratio)
	      mean_ratio += r;
	    mean_ratio /= model_error_ratio.size();
	    
	    if(mean_ratio > 1.0f){
	      char buffer[80];
	      snprintf(buffer, 80, "Optimizing program: SYNTH PREDICTOR ERROR LARGER THAN OUTPUT (%.2fx larger)", mean_ratio);
	      logging.warn(buffer);	    
	    }
	  }
	  
	  
	  // finds the best error
	  float best_error = 10e20;
	  for(unsigned int i=0;i<errors.size();i++){
	    if(errors[i].first < best_error){
	      soundParameters = synthTest;
	    }
	  }
	  
	}
	
	
	if((bestKeyword.size() <= 0 && keywordData.size() > 0) || bestPicture.size() <= 0){
		logging.error("Execute command couldn't find picture or keyword command to show (no models?)");
		engine_pollEvents();
		return false;
	}

	unsigned int keyword = 0;
	unsigned int picture = 0;

	{
		unsigned int elem = 0;

		if(keywordData.size() > 0){
			elem = rng.rand() % bestKeyword.size();
			for(auto& k : bestKeyword){
				if(elem <= 0){
					keyword = k.second;
					break;
				}
				else elem--;
			}
		}

		elem = rng.rand() % bestPicture.size();
		for(auto& p : bestPicture){
			if(elem <= 0){
				picture = p.second;
				break;
			}
			else elem--;
		}
	}


	if(keywordData.size() > 0)
	{
		char buffer[256];
		snprintf(buffer, 256, "prediction model selected keyword/best picture: %s %s",
				keywords[keyword].c_str(), pictures[picture].c_str());
		logging.info(buffer);
	}
	else{
		char buffer[256];
		snprintf(buffer, 256, "prediction model selected best picture: %s",
				pictures[picture].c_str());
		logging.info(buffer);
	}

	// now we have best picture and keyword that is predicted
	// to change users state to target value: show them

	if(keywordData.size() > 0){
		std::string message = keywords[keyword];
		engine_showScreen(message, picture, soundParameters);
	}
	else{
	        engine_showScreen(" ", picture, soundParameters);
	}

	engine_updateScreen();
	engine_pollEvents();

	return true;
}


// executes program blindly based on Monte Carlo sampling and prediction models
// [only works for low dimensional target signals and well-trained models]
bool ResonanzEngine::engine_executeProgramMonteCarlo(const std::vector<float>& eegTarget,
						     const std::vector<float>& eegTargetVariance, float timestep_)
{
	int bestKeyword = -1;
	int bestPicture = -1;
	float bestError = std::numeric_limits<double>::infinity();

	math::vertex<> target(eegTarget.size());
	math::vertex<> targetVariance(eegTargetVariance.size());
	whiteice::math::blas_real<float> timestep = timestep_;

	for(unsigned int i=0;i<target.size();i++){
		target[i] = eegTarget[i];
		targetVariance[i] = eegTargetVariance[i];
	}

	if(mcsamples.size() <= 0)
	        return false; // internal program error should have MC samples


	for(unsigned int index=0;index<keywordModels.size();index++){
		whiteice::bayesian_nnetwork<>& model = keywordModels[index];

		if(model.inputSize() != mcsamples[0].size() || model.outputSize() != eegTarget.size()){
			logging.warn("skipping bad keyword prediction model");
			continue; // bad model/data => ignore
		}

		// calculates average error for this model using MC samples
		float error = 0.0f;

#pragma omp parallel for
		for(unsigned int mcindex=0;mcindex<mcsamples.size();mcindex++){
			auto x = mcsamples[mcindex];

			if(keywordData[index].preprocess(0, x) == false){
				logging.warn("skipping bad keyword prediction model");
				continue;
			}

			math::vertex<> m;
			math::matrix<> cov;

			if(model.calculate(x, m, cov, 1, 0) == false){
				logging.warn("skipping bad keyword prediction model");
				continue;
			}

			if(keywordData[index].invpreprocess(1, m) == false){
				logging.warn("skipping bad keyword prediction model");
				continue;
			}

			m *= timestep; // corrects delta to given timelength
			cov *= timestep*timestep;

			// now we have prediction x to the response to the given keyword
			// calculates error (weighted distance to the target state)

			auto delta = target - (m + x);

			for(unsigned int i=0;i<delta.size();i++){
				delta[i] = math::abs(delta[i]) + math::sqrt(cov(i,i)); // Var[x - y] = Var[x] + Var[y]
				delta[i] /= targetVariance[i];
			}

			auto e = delta.norm();

			float ef = 0.0f;
			math::convert(ef, e);

#pragma omp critical(update_error1)
			{
				error += ef / mcsamples.size();
			}
		}

		if(error < bestError){
			bestError = error;
			bestKeyword = index;
		}

		engine_pollEvents(); // polls here for incoming events in case there are lots of models and this is slow..
	}


	bestError = std::numeric_limits<double>::infinity();

	for(unsigned int index=0;index<pictureModels.size();index++){
		whiteice::bayesian_nnetwork<>& model = pictureModels[index];

		if(model.inputSize() != mcsamples[0].size() || model.outputSize() != eegTarget.size()){
			logging.warn("skipping bad picture prediction model");
			continue; // bad model/data => ignore
		}

		// calculates average error for this model using MC samples
		float error = 0.0f;

#pragma omp parallel for
		for(unsigned int mcindex=0;mcindex<mcsamples.size();mcindex++){
			auto x = mcsamples[mcindex];

			if(pictureData[index].preprocess(0, x) == false){
				logging.warn("skipping bad picture prediction model");
				continue;
			}

			math::vertex<> m;
			math::matrix<> cov;

			if(model.calculate(x, m, cov, 1, 0) == false){
				logging.warn("skipping bad picture prediction model");
				continue;
			}

			if(pictureData[index].invpreprocess(1, m) == false){
				logging.warn("skipping bad picture prediction model");
				continue;
			}

			m *= timestep; // corrects delta to given timelength
			cov *= timestep*timestep;

			// now we have prediction x to the response to the given keyword
			// calculates error (weighted distance to the target state)

			auto delta = target - (m + x);

			for(unsigned int i=0;i<delta.size();i++){
				delta[i] = math::abs(delta[i]) + math::sqrt(cov(i,i)); // Var[x - y] = Var[x] + Var[y]
				delta[i] /= targetVariance[i];
			}

			auto e = delta.norm();

			float ef = 0.0f;
			math::convert(ef, e);

#pragma omp critical(update_error2)
			{
				error += ef / mcsamples.size();
			}
		}

		if(error < bestError){
			bestError = error;
			bestPicture = index;
		}

		engine_pollEvents(); // polls for incoming events in case there are lots of models
	}
	
	if(bestPicture < 0){
		logging.error("Execute command couldn't find picture to show (no models?)");
		engine_pollEvents();
		return false;
	}
	else{
	        if(bestKeyword >= 0 && bestPicture >= 0){
		  char buffer[80];
		  snprintf(buffer, 80, "prediction model selected keyword/best picture: %s %s",
			   keywords[bestKeyword].c_str(), pictures[bestPicture].c_str());
		  logging.info(buffer);
		}
		else{
		  char buffer[80];
		  snprintf(buffer, 80, "prediction model selected best picture: %s",
			   pictures[bestPicture].c_str());
		  logging.info(buffer);
		}
	}


	// calculates estimates for MCMC samples [user state after stimulus] for the next round
	// we have two separate prediction models for this: keywords and pictures:
	// * for each sample we decide randomly which prediction model to use.
	// * additionally, we now postprocess samples to stay within [0,1] interval
	//   as our eeg-metasignals are always within [0,1]-range
	{
		for(auto& x : mcsamples){

			if((rand()&1) == 0 && bestKeyword >= 0){ // use keyword model to predict the outcome
				const auto& index = bestKeyword;

				whiteice::bayesian_nnetwork<>& model = keywordModels[index];

				if(keywordData[index].preprocess(0, x) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}

				math::vertex<> m;
				math::matrix<> cov;

				if(model.calculate(x, m, cov, 1 ,0) == false){
					logging.warn("skipping bad keyword prediction model");
					continue;
				}

				if(keywordData[index].invpreprocess(1, m) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}

				m *= timestep; // corrects delta to given timelength
				cov *= timestep*timestep;

				// TODO take a MCMC sample from N(mean, cov)

				x = m + x; // just uses mean value here..
			}
			else{ // use picture model to predict the outcome
				const auto& index = bestPicture;

				whiteice::bayesian_nnetwork<>& model = pictureModels[index];

				if(pictureData[index].preprocess(0, x) == false){
					logging.error("mc sampling: skipping bad picture prediction model");
					continue;
				}

				math::vertex<> m;
				math::matrix<> cov;

				if(model.calculate(x, m, cov, 1, 0) == false){
					logging.warn("skipping bad picture prediction model");
					continue;
				}

				if(pictureData[index].invpreprocess(1, m) == false){
					logging.error("mc sampling: skipping bad picture prediction model");
					continue;
				}

				// FIXME if output is PCAed then covariance matrix should be preprocessed too

				m *= timestep; // corrects delta to given timelength
				cov *= timestep*timestep;

				// TODO take a MCMC sample from N(mean, cov)

				x = m + x; // just uses mean value here..
			}

			// post-process predicted x to be within [0,1] interval
			for(unsigned int i=0;i<x.size();i++){
				if(x[i] < 0.0f) x[i] = 0.0f;
				else if(x[i] > 1.0f) x[i] = 1.0f;
			}

			// 20% of the samples will be reset to random points (viewer state "resets")
			if(rng.uniform() < 0.20){
			  for(unsigned int j=0;j<x.size();j++)
			    x[j] = rng.uniform(); // [0,1] valued signals sampled from [0,1]^D
			}

			engine_pollEvents(); // polls for incoming events in case there are lots of samples
		}
	}

	// now we have best picture and keyword that is predicted
	// to change users state to target value: show them	

	{
	  std::vector<float> synthParams;
	  if(synth){
	    synthParams.resize(synth->getNumberOfParameters());
	    for(unsigned int i=0;i<synthParams.size();i++)
	      synthParams[i] = 0.0f;
	  }

	  if(bestKeyword >= 0){
	    std::string message = keywords[bestKeyword];
	    engine_showScreen(message, bestPicture, synthParams);
	  }
	  else{
	    std::string message = " ";
	    engine_showScreen(message, bestPicture, synthParams);
	  }
	}

	engine_updateScreen();
	engine_pollEvents();

	return true;
}


bool ResonanzEngine::engine_optimizeModels(unsigned int& currentPictureModel, 
					   unsigned int& currentKeywordModel, 
					   bool& soundModelCalculated)
{
        // always first optimizes sound model

        if(synth == NULL && soundModelCalculated == false){
	  soundModelCalculated = true; // we skip synth optimization
	  logging.info("Audio/synth is disabled so skipping synthesizer optimizations");
	  nnsynth->randomize();
	}
        
        if(soundModelCalculated == false){
	  
	        // first model to be optimized (no need to store the previous result)
		if(optimizer == nullptr && bayes_optimizer == nullptr){
			whiteice::math::vertex<> w;
			
			nnsynth->randomize();
			nnsynth->exportdata(w);
			
			{
			  char buffer[512];
			  snprintf(buffer, 512, "resonanz model optimization started: synth model. database size: %d %d",
				   synthData.size(0), synthData.size(1));
			  logging.info(buffer);
			}


			optimizer = new whiteice::pLBFGS_nnetwork<>(*nnsynth, synthData, false, false);
			optimizer->minimize(NUM_OPTIMIZER_THREADS);
		}
		else if(optimizer != nullptr && use_bayesian_nnetwork){ // pre-optimizer is active

			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(iterations >= NUM_OPTIMIZER_ITERATIONS){
				// gets finished solution
			  
			        logging.info("DEBUG: ABOUT TO STOP COMPUTATION");

				optimizer->stopComputation();
				optimizer->getSolution(w, error, iterations);
				
				{
				  char buffer[512];
				  snprintf(buffer, 512, "resonanz model lbfgs optimization stopped. synth model. iterations: %d error: %f",
					   iterations, error.c[0]);
				  logging.info(buffer);
				}

				nnsynth->importdata(w);

				// switches to uncertainty analysis
				const bool adaptive = true;
				
				logging.info("DEBUG: STARTING HMC SAMPLER");

				bayes_optimizer = new whiteice::UHMC<>(*nnsynth, synthData, adaptive);
				bayes_optimizer->startSampler();

				delete optimizer;
				optimizer = nullptr;
			}
			else{
				{
				  math::vertex< math::blas_real<float> > w;
				  math::blas_real<float> error;
				  unsigned int iterations = 0;
				  
				  optimizer->getSolution(w, error, iterations);
				  
				  char buffer[512];
				  snprintf(buffer, 512, "resonanz L-BFGS model optimization running. synth model. number of iterations: %d/%d. error: %f",
					   iterations, NUM_OPTIMIZER_ITERATIONS, error.c[0]);
				  logging.info(buffer);
				}
			}

		}
		else if(bayes_optimizer != nullptr){
			if(bayes_optimizer->getNumberOfSamples() >= BAYES_NUM_SAMPLES){ // time to stop computation [got uncertainly information]

				bayes_optimizer->stopSampler();

				{
					char buffer[512];
					snprintf(buffer, 512, "resonanz bayes synth model optimization stopped. iterations: %d",
						 bayes_optimizer->getNumberOfSamples());
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
				  calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".model";

				bayes_optimizer->getNetwork(*bnn);

				if(bnn->save(dbFilename) == false)
					logging.error("saving bayesian nn configuration file failed");

				delete bayes_optimizer;
				bayes_optimizer = nullptr;
				
				soundModelCalculated = true;
			}
			else{
			        unsigned int samples = bayes_optimizer->getNumberOfSamples();
				
				{
				  char buffer[512];
					snprintf(buffer, 512, "resonanz bayes model optimization running. synth model. number of samples: %d/%d",
						 bayes_optimizer->getNumberOfSamples(), BAYES_NUM_SAMPLES);
					logging.info(buffer);
				}
				
			}
		}
		else if(optimizer != nullptr){
			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(iterations >= NUM_OPTIMIZER_ITERATIONS){
				// gets finished solution

				optimizer->stopComputation();
				optimizer->getSolution(w, error, iterations);

				{
					char buffer[512];
					snprintf(buffer, 512, "resonanz model optimization stopped. synth model. iterations: %d error: %f",
						 iterations, error.c[0]);
					logging.info(buffer);
				}
				
				// saves optimization results to a file
				std::string modelFilename = currentCommand.modelDir + "/" + 
				  calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".model";

				nnsynth->importdata(w);

				bnn->importNetwork(*nnsynth);

				if(bnn->save(modelFilename) == false)
					logging.error("saving nn configuration file failed");

				delete optimizer;
				optimizer = nullptr;

				soundModelCalculated = true;
			}
			else{
			  {
			    math::vertex< math::blas_real<float> > w;
			    math::blas_real<float> error;
			    unsigned int iterations = 0;
			    
			    optimizer->getSolution(w, error, iterations);
			    
			    char buffer[512];
			    snprintf(buffer, 512, "resonanz L-BFGS model optimization running. synth model. number of iterations: %d/%d. error: %f",
				     iterations, NUM_OPTIMIZER_ITERATIONS, error.c[0]);
			    logging.info(buffer);
			  }
			}
		}
	  
	  
	}
  	else if(currentPictureModel < pictureData.size() && optimizeSynthOnly == false){

		// first model to be optimized (no need to store the previous result)
		if(optimizer == nullptr && bayes_optimizer == nullptr){
			whiteice::math::vertex<> w;

			nn->randomize();
			nn->exportdata(w);
			
			optimizer = new whiteice::pLBFGS_nnetwork<>(*nn, pictureData[currentPictureModel], false, false);
			optimizer->minimize(NUM_OPTIMIZER_THREADS);

			{
				char buffer[512];
				snprintf(buffer, 512, "resonanz model optimization started: picture %d database size: %d",
						currentPictureModel, pictureData[currentPictureModel].size(0));
				logging.info(buffer);
			}

		}
		else if(optimizer != nullptr && use_bayesian_nnetwork){ // pre-optimizer is active

			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(iterations >= NUM_OPTIMIZER_ITERATIONS){
				// gets finished solution

				optimizer->stopComputation();
				optimizer->getSolution(w, error, iterations);
				
				{
				  char buffer[512];
				  snprintf(buffer, 512, "resonanz model lbfgs optimization stopped. picture model. iterations: %d error: %f",
					   iterations, error.c[0]);
				  logging.info(buffer);
				}

				nn->importdata(w);

				// switches to uncertainty analysis
				const bool adaptive = true;

				bayes_optimizer = new whiteice::UHMC<>(*nn, pictureData[currentPictureModel], adaptive);
				bayes_optimizer->startSampler();

				delete optimizer;
				optimizer = nullptr;
			}
			else{
				{
				  char buffer[512];
					snprintf(buffer, 512, "resonanz L-BFGS model optimization running. picture model. number of iterations: %d/%d",
						 optimizer->getIterations(), NUM_OPTIMIZER_ITERATIONS);
					logging.info(buffer);
				}
			}

		}
		else if(bayes_optimizer != nullptr){
			if(bayes_optimizer->getNumberOfSamples() >= BAYES_NUM_SAMPLES){ // time to stop computation [got uncertainly information]

				bayes_optimizer->stopSampler();

				{
					char buffer[512];
					snprintf(buffer, 512, "resonanz bayes model optimization stopped. picture: %d iterations: %d",
							currentPictureModel, bayes_optimizer->getNumberOfSamples());
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
						calculateHashName(pictures[currentPictureModel] + eeg->getDataSourceName()) + ".model";

				bayes_optimizer->getNetwork(*bnn);

				if(bnn->save(dbFilename) == false)
					logging.error("saving bayesian nn configuration file failed");

				delete bayes_optimizer;
				bayes_optimizer = nullptr;

				// starts new computation
				currentPictureModel++;
				if(currentPictureModel < pictures.size()){
					{
						char buffer[512];
						snprintf(buffer, 512, "resonanz bayes model optimization started: picture %d database size: %d",
								currentPictureModel, pictureData[currentPictureModel].size(0));
						logging.info(buffer);
					}

					// actives pre-optimizer
					whiteice::math::vertex<> w;

					nn->randomize();
					nn->exportdata(w);

					optimizer = new whiteice::pLBFGS_nnetwork<>(*nn, pictureData[currentPictureModel], false, false);
					optimizer->minimize(NUM_OPTIMIZER_THREADS);

				}
			}
			else{
			        unsigned int samples = bayes_optimizer->getNumberOfSamples();
				
				if((samples % 100) == 0){
				  char buffer[512];
					snprintf(buffer, 512, "resonanz bayes model optimization running. picture: %d number of samples: %d/%d",
						 currentPictureModel, bayes_optimizer->getNumberOfSamples(), BAYES_NUM_SAMPLES);
					logging.info(buffer);
				}
				
			}
		}
		else if(optimizer != nullptr){
			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(iterations >= NUM_OPTIMIZER_ITERATIONS){
				// gets finished solution

				optimizer->stopComputation();
				optimizer->getSolution(w, error, iterations);

				{
					char buffer[512];
					snprintf(buffer, 512, "resonanz model optimization stopped. picture: %d iterations: %d error: %f",
							currentPictureModel, iterations, error.c[0]);
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
						calculateHashName(pictures[currentPictureModel] + eeg->getDataSourceName()) + ".model";
				nn->importdata(w);

				bnn->importNetwork(*nn);

				if(bnn->save(dbFilename) == false)
					logging.error("saving nn configuration file failed");

				delete optimizer;
				optimizer = nullptr;

				// starts new computation
				currentPictureModel++;
				if(currentPictureModel < pictures.size()){
					nn->randomize();
					nn->exportdata(w);

					{
						char buffer[512];
						snprintf(buffer, 512, "resonanz model optimization started: picture %d database size: %d",
								currentPictureModel, pictureData[currentPictureModel].size(0));
						logging.info(buffer);
					}

					optimizer = new whiteice::pLBFGS_nnetwork<>(*nn, pictureData[currentPictureModel], false, false);
					optimizer->minimize(NUM_OPTIMIZER_THREADS);
				}
			}
		}
		else{
		  {
		    math::vertex< math::blas_real<float> > w;
		    math::blas_real<float> error;
		    unsigned int iterations = 0;
		    
		    optimizer->getSolution(w, error, iterations);
		    
		    char buffer[512];
		    snprintf(buffer, 512, "resonanz L-BFGS model optimization running. picture model %d/%d. number of iterations: %d/%d. error: %f",
			     currentPictureModel, pictures.size(), 
			     iterations, NUM_OPTIMIZER_ITERATIONS, error.c[0]);
		    logging.info(buffer);
		  }
		}

	}
	else if(currentKeywordModel < keywords.size() && optimizeSynthOnly == false){

		if(optimizer == nullptr && bayes_optimizer == nullptr){ // first model to be optimized (no need to store the previous result)

			whiteice::math::vertex<> w;

			nn->randomize();
			nn->exportdata(w);

			optimizer = new whiteice::pLBFGS_nnetwork<>(*nn, keywordData[currentKeywordModel], false, false);
			optimizer->minimize(NUM_OPTIMIZER_THREADS);

			{
				char buffer[512];
				snprintf(buffer, 512, "resonanz model optimization started: keyword %d database size: %d",
							currentKeywordModel, keywordData[currentKeywordModel].size(0));
				logging.info(buffer);
			}

		}
		else if(optimizer != nullptr && use_bayesian_nnetwork){ // pre-optimizer is active

			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(iterations >= NUM_OPTIMIZER_ITERATIONS){
				// gets finished solution

				optimizer->stopComputation();
				optimizer->getSolution(w, error, iterations);
				
				{
				  char buffer[512];
				  snprintf(buffer, 512, "resonanz model lbfgs optimization stopped. keyword model. iterations: %d error: %f",
					   iterations, error.c[0]);
				  logging.info(buffer);
				}

				nn->importdata(w);

				// switches to bayesian optimizer
				const bool adaptive = true;

				bayes_optimizer = new whiteice::UHMC<>(*nn, keywordData[currentKeywordModel], adaptive);
				bayes_optimizer->startSampler();

				delete optimizer;
				optimizer = nullptr;
			}
			else{
				{
				        char buffer[512];
				        snprintf(buffer, 512, "resonanz L-BFGS model optimization running. keyword model. number of iterations: %d/%d",
						 optimizer->getIterations(), NUM_OPTIMIZER_ITERATIONS);
					logging.info(buffer);
				}
			}

		}
		else if(bayes_optimizer != nullptr){
			if(bayes_optimizer->getNumberOfSamples() >= BAYES_NUM_SAMPLES){ // time to stop computation

				bayes_optimizer->stopSampler();

				{
					char buffer[512];
					snprintf(buffer, 512, "resonanz bayes model optimization stopped. keyword: %d iterations: %d",
							currentKeywordModel, bayes_optimizer->getNumberOfSamples());
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
						calculateHashName(keywords[currentKeywordModel] + eeg->getDataSourceName()) + ".model";

				bayes_optimizer->getNetwork(*bnn);

				if(bnn->save(dbFilename) == false)
					logging.error("saving bayesian nn configuration file failed");

				delete bayes_optimizer;
				bayes_optimizer = nullptr;

				// starts new computation
				currentKeywordModel++;
				if(currentKeywordModel < keywords.size()){
					{
						char buffer[512];
						snprintf(buffer, 512, "resonanz bayes model optimization started: keyword %d database size: %d",
								currentKeywordModel, keywordData[currentKeywordModel].size(0));
						logging.info(buffer);
					}

					whiteice::math::vertex<> w;
					nn->randomize();
					nn->exportdata(w);

					optimizer = new whiteice::pLBFGS_nnetwork<>(*nn, keywordData[currentKeywordModel], false, false);
					optimizer->minimize(NUM_OPTIMIZER_THREADS);
				}
			}
			else{
			        unsigned int samples = bayes_optimizer->getNumberOfSamples();
				
				if((samples % 100) == 0){
				  char buffer[512];
					snprintf(buffer, 512, "resonanz bayes model optimization running. keyword: %d number of samples: %d/%d",
						 currentKeywordModel, bayes_optimizer->getNumberOfSamples(), BAYES_NUM_SAMPLES);
					logging.info(buffer);
				}
				
			}
		}
		else if(optimizer != nullptr){
			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(iterations >= NUM_OPTIMIZER_ITERATIONS){
				// gets finished solution

				optimizer->stopComputation();

				optimizer->getSolution(w, error, iterations);


				{
					char buffer[512];
					snprintf(buffer, 512, "resonanz model optimization stopped. keyword: %d iterations: %d error: %f",
						 currentKeywordModel, iterations, error.c[0]);
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
						calculateHashName(keywords[currentKeywordModel] + eeg->getDataSourceName()) + ".model";
				nn->importdata(w);

				bnn->importNetwork(*nn);

				if(bnn->save(dbFilename) == false)
					logging.error("saving nn configuration file failed");

				delete optimizer;
				optimizer = nullptr;

				// starts new computation
				currentKeywordModel++;
				if(currentKeywordModel < keywords.size()){
					nn->randomize();
					nn->exportdata(w);

					{
						char buffer[512];
						snprintf(buffer, 512, "resonanz model optimization started: keyword %d database size: %d",
								currentKeywordModel, keywordData[currentKeywordModel].size(0));
						logging.info(buffer);
					}

					optimizer = new whiteice::pLBFGS_nnetwork<>(*nn, keywordData[currentKeywordModel], false, false);
					optimizer->minimize(NUM_OPTIMIZER_THREADS);
				}
			}
		}
		else{
		  {
		    math::vertex< math::blas_real<float> > w;
		    math::blas_real<float> error;
		    unsigned int iterations = 0;
		    
		    optimizer->getSolution(w, error, iterations);
		    
		    char buffer[512];
		    snprintf(buffer, 512, "resonanz L-BFGS model optimization running. keyword model %d/%d. number of iterations: %d/%d. error: %f",
			     currentKeywordModel, keywords.size(), 
			     iterations, NUM_OPTIMIZER_ITERATIONS, error.c[0]);
		    logging.info(buffer);
		  }
		}
	}
	else{ // both synth, picture and keyword models has been computed or
	      // optimizeSynthOnly == true and only synth model has been computed => stop
		cmdStopCommand();
	}

	return true;
}



// estimate output value N(m,cov) for x given dataset data uses nearest neighbourhood estimation (distance)
bool ResonanzEngine::engine_estimateNN(const whiteice::math::vertex<>& x, const whiteice::dataset<>& data,
		whiteice::math::vertex<>& m, whiteice::math::matrix<>& cov)
{
  bool bad_data = false;
  
  if(data.size(0) <= 1) bad_data = true;
  if(data.getNumberOfClusters() != 2) bad_data = true;
  if(bad_data == false){
    if(data.size(0) != data.size(1))
      bad_data = true;
  }
  
  if(bad_data){
    m = x;
    cov.resize(x.size(), x.size());
    cov.identity();
    
    return true;
  }
  
  m.resize(x.size());
  m.zero();
  cov.resize(x.size(),x.size());
  cov.zero();
  const float epsilon    = 1.0f;
  math::blas_real<float> sumweights = 0.0f;
  
  for(unsigned int i=0;i<data.size(0);i++){
    auto delta = x - data.access(0, i);
    auto w = math::blas_real<float>(1.0f / (epsilon + delta.norm().c[0]));
	  
    auto v = data.access(1, i);
    
    m += w*v;
    cov += w*v.outerproduct();
    
    sumweights += w;
  }
  
  m /= sumweights;
  cov /= sumweights;
  
  cov -= m.outerproduct();
  
  return true;
}



void ResonanzEngine::engine_setStatus(const std::string& msg) throw()
{
	try{
		std::lock_guard<std::mutex> lock(status_mutex);
		engineState = msg;
		logging.info(msg);
	}
	catch(std::exception& e){ }
}


void ResonanzEngine::engine_sleep(int msecs)
{
	// sleeps for given number of milliseconds

	// currently just sleeps()
	std::chrono::milliseconds duration(msecs);
	std::this_thread::sleep_for(duration);
}


bool ResonanzEngine::engine_checkIncomingCommand()
{
	logging.info("checking command");
	
	if(incomingCommand == nullptr) return false;
	
	std::lock_guard<std::mutex> lock(command_mutex);

	if(incomingCommand == nullptr) 
	  return false; // incomingCommand changed while acquiring mutex..

	currentCommand = *incomingCommand;

	delete incomingCommand;
	incomingCommand = nullptr;

	return true;
}


bool ResonanzEngine::engine_loadMedia(const std::string& picdir, const std::string& keyfile, bool loadData)
{
	std::vector<std::string> tempKeywords;

	// first we get picture names from directories and keywords from keyfile
	if(this->loadWords(keyfile, tempKeywords) == false){
	  logging.warn("loading keyword file FAILED.");
	}

#if 0
	if(this->loadWords(keyfile, tempKeywords) == false){
	  logging.error("loading keyword file FAILED.");
	  return false;
	}
#endif
		

	std::vector<std::string> tempPictures;

	if(this->loadPictures(picdir, tempPictures) == false){
	  logging.error("loading picture filenames FAILED.");
	  return false;
	}
		

	pictures = tempPictures;
	keywords = tempKeywords;

	for(unsigned int i=0;i<images.size();i++){
		if(images[i] != nullptr)
			SDL_FreeSurface(images[i]);
		images[i] = nullptr;
	}

	if(loadData){
		images.resize(pictures.size());
		
		std::vector<float> synthParams;
		if(synth){
		  synthParams.resize(synth->getNumberOfParameters());
		  for(unsigned int i=0;i<synthParams.size();i++)
		    synthParams[i] = 0.0f;
		}
		
		
		for(unsigned int i=0;i<images.size();i++){
			images[i] = nullptr;
			
			char buffer[80];
			
			snprintf(buffer, 80, "resonanz-engine: loading media files (%.1f%%)..",
				 100.0f*(((float)i)/((float)images.size())));
			engine_setStatus(buffer);
			
			engine_showScreen("Loading..", i, synthParams);
			
			engine_pollEvents();
			engine_updateScreen();
		}
		
		engine_pollEvents();
		engine_updateScreen();
	}
	else{
		images.clear();
	}

	return true;
}


bool ResonanzEngine::engine_loadDatabase(const std::string& modelDir)
{
	std::lock_guard<std::mutex> lock(database_mutex);

	keywordData.resize(keywords.size());
	pictureData.resize(pictures.size());

	std::string name1 = "input";
	std::string name2 = "output";
	
	float keyword_num_samples = 0.0f;
	float picture_num_samples = 0.0f;
	float synth_num_samples   = 0.0f;

	// loads databases into memory or initializes new ones
	for(unsigned int i=0;i<keywords.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";
		
		keywordData[i].clear();

		if(keywordData[i].load(dbFilename) == false){
			keywordData[i].createCluster(name1, eeg->getNumberOfSignals());
			keywordData[i].createCluster(name2, eeg->getNumberOfSignals());
			logging.info("Couldn't load keyword data => creating empty database");
		}
		else{
		  if(keywordData[i].getNumberOfClusters() != 2){
		    logging.error("Keyword data wrong number of clusters or data corruption => reset database");
		    keywordData[i].clear();
		    keywordData[i].createCluster(name1, eeg->getNumberOfSignals());
		    keywordData[i].createCluster(name2, eeg->getNumberOfSignals());
		  }
		}
		
		const unsigned int datasize = keywordData[i].size(0);
		
		if(keywordData[i].removeBadData() == false)
		  logging.warn("keywordData: bad data removal failed");
		
		if(datasize != keywordData[i].size(0)){
		  char buffer[80];
		  snprintf(buffer, 80, "Keyword %d: bad data removal reduced data: %d => %d\n",
			   i, datasize, keywordData[i].size(0));
		  logging.warn(buffer);
		}
		
		keyword_num_samples += keywordData[i].size(0);

		{
			if(pcaPreprocess){
				if(keywordData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing keyword measurements [input]");
					keywordData[i].preprocess(0, whiteice::dataset<>::dnCorrelationRemoval);
				}
#if 0
				if(keywordData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing keyword measurements [output]");
					keywordData[i].preprocess(1, whiteice::dataset<>::dnCorrelationRemoval);
				}
#endif
				keywordData[i].convert(1);
			}
			else{
				if(keywordData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA processing of keyword measurements [input]");
					keywordData[i].convert(0); // removes all preprocessings from input
				}
#if 0
				if(keywordData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA processing of keyword measurements [output]");
					keywordData[i].convert(1); // removes all preprocessings from input
				}
#endif
				keywordData[i].convert(1);
			}

			// keywordData[i].convert(1); // removes all preprocessings from output
		}
		
	}
	
	logging.info("keywords measurement database loaded");



	for(unsigned int i=0;i<pictures.size();i++){
	        std::string dbFilename = modelDir + "/" + calculateHashName(pictures[i] + eeg->getDataSourceName()) + ".ds";
		
		pictureData[i].clear();

		if(pictureData[i].load(dbFilename) == false){
			pictureData[i].createCluster(name1, eeg->getNumberOfSignals());
			pictureData[i].createCluster(name2, eeg->getNumberOfSignals());
			logging.info("Couldn't load picture data => creating empty database");
		}
		else{
		  if(pictureData[i].getNumberOfClusters() != 2){
		    logging.error("Picture data wrong number of clusters or data corruption => reset database");
		    pictureData[i].clear();
		    pictureData[i].createCluster(name1, eeg->getNumberOfSignals());
		    pictureData[i].createCluster(name2, eeg->getNumberOfSignals());
		  }
		}
		
		const unsigned int datasize = pictureData[i].size(0);
		
		if(pictureData[i].removeBadData() == false)
		  logging.warn("pictureData: bad data removal failed");
		
		if(datasize != pictureData[i].size(0)){
		  char buffer[80];
		  snprintf(buffer, 80, "Picture %d: bad data removal reduced data: %d => %d\n",
			   i, datasize, pictureData[i].size(0));
		  logging.warn(buffer);
		}
		
		picture_num_samples += pictureData[i].size(0);
		

		{
			if(pcaPreprocess){
				if(pictureData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing picture measurements [input]");
					pictureData[i].preprocess(0, whiteice::dataset<>::dnCorrelationRemoval);
				}
#if 0
				if(pictureData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing picture measurements [output]");
					pictureData[i].preprocess(1, whiteice::dataset<>::dnCorrelationRemoval);
				}
#endif
				pictureData[i].convert(1);
			}
			else{
				if(pictureData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA processing of picture measurements [input]");
					pictureData[i].convert(0); // removes all preprocessings from input
				}
#if 0
				if(pictureData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA processing of picture measurements [output]");
					pictureData[i].convert(1); // removes all preprocessings from input
				}
#endif
				pictureData[i].convert(1);
			}

			// pictureData[i].convert(1); // removes all preprocessings from output
		}
	}
	
	logging.info("picture measurement database loaded");
	
	// loads synth parameters data into memory
	if(synth){
	  std::string dbFilename = modelDir + "/" + calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".ds";
	  
	  synthData.clear();
	  
	  if(synthData.load(dbFilename) == false){
	    synthData.createCluster(name1, eeg->getNumberOfSignals() + 2*synth->getNumberOfParameters());
	    synthData.createCluster(name2, eeg->getNumberOfSignals());
	    logging.info("Couldn't load synth data => creating empty database");
	  }
	  else{
	    if(synthData.getNumberOfClusters() != 2){
	      logging.error("Synth data wrong number of clusters or data corruption => reset database");
	      synthData.clear();
	      synthData.createCluster(name1, eeg->getNumberOfSignals() + 2*synth->getNumberOfParameters());
	      synthData.createCluster(name2, eeg->getNumberOfSignals());
	    }
	  }
	  
	  const unsigned int datasize = synthData.size(0);
	  
	  if(synthData.removeBadData() == false)
	    logging.warn("synthData: bad data removal failed");
	  
	  if(datasize != synthData.size(0)){
	    char buffer[80];
	    snprintf(buffer, 80, "Synth data: bad data removal reduced data: %d => %d\n",
		     datasize, synthData.size(0));
	    logging.warn(buffer);
	  }
	  
	  synth_num_samples += synthData.size(0);
	  
	  {
	    if(pcaPreprocess){
	      if(synthData.hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == false){
		logging.info("PCA preprocessing sound measurements [input]");
		synthData.preprocess(0, whiteice::dataset<>::dnCorrelationRemoval);
	      }
	      
	      synthData.convert(1);
	    }
	    else{
	      if(synthData.hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == true){
		logging.info("Removing PCA processing of sound measurements [input]");
		synthData.convert(0); // removes all preprocessings from input
	      }
	      
	      synthData.convert(1);
	    }
	  }
	  
	  logging.info("synth measurement database loaded");
	}
	
	
	// reports average samples in each dataset
	{
	  if(keywordData.size() > 0){
	    keyword_num_samples /= keywordData.size();
	  }
	  else{
	    keyword_num_samples = 0.0f;
	  }
	  
	  picture_num_samples /= pictureData.size();
	  synth_num_samples   /= 1;
	  
	  char buffer[128];
	  snprintf(buffer, 128, 
		   "measurements database loaded: %.1f [samples/picture] %.1f [samples/keyword] %.1f [synth samples]", picture_num_samples, keyword_num_samples, synth_num_samples);
	  
	  logging.info(buffer);
	}
	
	

	return true;
}


bool ResonanzEngine::engine_storeMeasurement(unsigned int pic, unsigned int key, 
					     const std::vector<float>& eegBefore, 
					     const std::vector<float>& eegAfter,
					     const std::vector<float>& synthBefore,
					     const std::vector<float>& synthAfter)
{
        std::vector< whiteice::math::blas_real<float> > t1, t2;
	t1.resize(eegBefore.size());
	t2.resize(eegAfter.size());
	
	if(t1.size() != t2.size())
	  return false;
	
	// initialize to zero [no bad data possible]
	for(auto& t : t1) t = 0.0f; 
	for(auto& t : t2) t = 0.0f;

	// heavy checks against correctness of the data because buggy code/hardware
	// seem to introduce bad measurment data into database..

	const whiteice::math::blas_real<float> delta = MEASUREMODE_DELAY_MS/1000.0f;

	for(unsigned int i=0;i<t1.size();i++){
	        auto& before = eegBefore[i];
		auto& after  = eegAfter[i];
		
		if(before < 0.0f){
		  logging.error("store measurement. bad data: eegBefore < 0.0");
		  return false;
		}
		else if(before > 1.0f){
		  logging.error("store measurement. bad data: eegBefore > 1.0");
		  return false;
		}
		else if(whiteice::math::isnan(before) || whiteice::math::isinf(before)){
		  logging.error("store measurement. bad data: eegBefore is NaN or Inf");
		  return false;
		}

		if(after < 0.0f){
		  logging.error("store measurement. bad data: eegAfter < 0.0");
		  return false;
		}
		else if(after > 1.0f){
		  logging.error("store measurement. bad data: eegAfter > 1.0");
		  return false;
		}
		else if(whiteice::math::isnan(after) || whiteice::math::isinf(after)){
		  logging.error("store measurement. bad data: eegAfter is NaN or Inf");
		  return false;
		}

	        
		t1[i] = eegBefore[i];
		t2[i] = (eegAfter[i] - eegBefore[i])/delta; // stores aprox "derivate": dEEG/dt
	}

	if(key < keywordData.size()){
	  if(keywordData[key].add(0, t1) == false || keywordData[key].add(1, t2) == false){
	    logging.error("Adding new keyword data FAILED");
	    return false;
	  }
	}

	if(pic < pictureData.size()){
	  if(pictureData[pic].add(0, t1) == false || pictureData[pic].add(1, t2) == false){
	    logging.error("Adding new picture data FAILED");
	    return false;
	  }
	}
	
	if(synth){
	  std::vector< whiteice::math::blas_real<float> > input, output;
	  
	  input.resize(synthBefore.size() + synthAfter.size() + eegBefore.size());
	  output.resize(eegAfter.size());
	  
	  // initialize to zero [no bad data possible]
	  for(auto& t : input)  t = 0.0f; 
	  for(auto& t : output) t = 0.0f;
	  
	  for(unsigned int i=0;i<synthBefore.size();i++){
	    if(synthBefore[i] < 0.0f){
	      logging.error("store measurement. bad data: synthBefore < 0.0");
	      return false;
	    }
	    else if(synthBefore[i] > 1.0f){
	      logging.error("store measurement. bad data: synthBefore > 1.0");
	      return false;
	    }
	    else if(whiteice::math::isnan(synthBefore[i]) || whiteice::math::isinf(synthBefore[i])){
	      logging.error("store measurement. bad data: synthBefore is NaN or Inf");
	      return false;
	    }
	      
	    input[i] = synthBefore[i];
	  }
	  
	  for(unsigned int i=0;i<synthAfter.size();i++){
	    if(synthAfter[i] < 0.0f){
	      logging.error("store measurement. bad data: synthAfter < 0.0");
	      return false;
	    }
	    else if(synthAfter[i] > 1.0f){
	      logging.error("store measurement. bad data: synthAfter > 1.0");
	      return false;
	    }
	    else if(whiteice::math::isnan(synthAfter[i]) || whiteice::math::isinf(synthAfter[i])){
	      logging.error("store measurement. bad data: synthAfter is NaN or Inf");
	      return false;
	    }
		    
	    input[synthBefore.size() + i] = synthAfter[i];
	  }
	  
	  for(unsigned int i=0;i<eegBefore.size();i++){
	    input[synthBefore.size()+synthAfter.size() + i] = eegBefore[i];
	  }
	  
	  for(unsigned int i=0;i<eegAfter.size();i++){
	    output[i] = (eegAfter[i] - eegBefore[i])/delta; // dEEG/dt
	  }
	  
	  if(synthData.add(0, input) == false || synthData.add(1, output) == false){
	    logging.error("Adding new synth data FAILED");
	    return false;
	  }
	}

	return true;
}


bool ResonanzEngine::engine_saveDatabase(const std::string& modelDir)
{
	std::lock_guard<std::mutex> lock(database_mutex);

	// saves databases from memory
	for(unsigned int i=0;i<keywordData.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";
		
		if(keywordData[i].removeBadData() == false)
		  logging.warn("keywordData: bad data removal failed");
		
		
		{
			if(pcaPreprocess){
				if(keywordData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing keyword measurements data [input]");
					keywordData[i].preprocess(0, whiteice::dataset<>::dnCorrelationRemoval);
				}
#if 0
				if(keywordData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing keyword measurements data [output]");
					keywordData[i].preprocess(1, whiteice::dataset<>::dnCorrelationRemoval);
				}
#endif
				keywordData[i].convert(1); // removes preprocessigns from output
			}
			else{
				if(keywordData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA preprocessing from keyword measurements data [input]");
					keywordData[i].convert(0); // removes all preprocessings from input
				}

#if 0
				if(keywordData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA preprocessing from keyword measurements data [output]");
					keywordData[i].convert(1); // removes all preprocessings from input
				}
#endif
				keywordData[i].convert(1); // removes preprocessigns from output
			}

			// keywordData[i].convert(1); // removes all preprocessings from output
		}

		if(keywordData[i].save(dbFilename) == false){
		  logging.error("Saving keyword data failed");
		  return false;
		}
	}


	for(unsigned int i=0;i<pictureData.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(pictures[i] + eeg->getDataSourceName()) + ".ds";
		
		if(pictureData[i].removeBadData() == false)
		  logging.warn("pictureData: bad data removal failed");
		
		
		{
			if(pcaPreprocess){
				if(pictureData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing picture measurements data [input]");
					pictureData[i].preprocess(0, whiteice::dataset<>::dnCorrelationRemoval);
				}
#if 0
				if(pictureData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == false){
					logging.info("PCA preprocessing picture measurements data [output]");
					pictureData[i].preprocess(1, whiteice::dataset<>::dnCorrelationRemoval);
				}
#endif
				pictureData[i].convert(1); // removes preprocessigns from output
			}
			else{
				if(pictureData[i].hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA preprocessing from picture measurements data [input]");
					pictureData[i].convert(0); // removes all preprocessings from input
				}
#if 0
				if(pictureData[i].hasPreprocess(1, whiteice::dataset<>::dnCorrelationRemoval) == true){
					logging.info("Removing PCA preprocessing from picture measurements data [output]");
					pictureData[i].convert(1); // removes all preprocessings from input
				}
#endif
				pictureData[i].convert(1);
			}

			// pictureData[i].convert(1); // removes all preprocessings from output
		}

		if(pictureData[i].save(dbFilename) == false){
		  logging.error("Saving picture data failed");
		  return false;
		}
	}

	// stores sound synthesis measurements
	if(synth){
	  std::string dbFilename = modelDir + "/" + calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".ds";
	  
	  if(synthData.removeBadData() == false)
	    logging.warn("synthData: bad data removal failed");
	  
	  {
	    if(pcaPreprocess){
	      if(synthData.hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == false){
		logging.info("PCA preprocessing sound measurements [input]");
		synthData.preprocess(0, whiteice::dataset<>::dnCorrelationRemoval);
	      }
	      
	      synthData.convert(1);
	    }
	    else{
	      if(synthData.hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval) == true){
		logging.info("Removing PCA processing of sound measurements [input]");
		synthData.convert(0); // removes all preprocessings from input
	      }
	      
	      synthData.convert(1);
	    }
	  }
	  
	  if(synthData.save(dbFilename) == false){
	    logging.info("Saving synth data failed");
	    return false;
	  }
	}


	return true;
}


std::string ResonanzEngine::calculateHashName(const std::string& filename) const
{
	try{
		unsigned int N = strlen(filename.c_str()) + 1;
		unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char)*N);
		unsigned char* hash160 = (unsigned char*)malloc(sizeof(unsigned char)*20);

		whiteice::crypto::SHA sha(160);

		memcpy(data, filename.c_str(), N);

		if(sha.hash(&data, N, hash160)){
			std::string result = "";
			for(unsigned int i=0;i<20;i++){
				char buffer[10];
				snprintf(buffer, 10, "%.2x", hash160[i]);
				result += buffer;
			}

			if(data) free(data);
			if(hash160) free(hash160);

			// printf("%s => %s\n", filename.c_str(), result.c_str());

			return result; // returns hex hash of the name
		}
		else{
			if(data) free(data);
			if(hash160) free(hash160);

			return "";
		}
	}
	catch(std::exception& e){
		return "";
	}
}


bool ResonanzEngine::engine_showScreen(const std::string& message, unsigned int picture,
				       const std::vector<float>& synthParams)
{
        SDL_Surface* surface = SDL_GetWindowSurface(window);
	if(surface == nullptr)
		return false;

	if(SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0)) != 0)
	  return false;

	int bgcolor = 0;
	int elementsDisplayed = 0;

	{
	  char buffer[256];
	  snprintf(buffer, 256, "engine_showScreen(%s %d/%d dim(%d)) called",
		   message.c_str(), picture, pictures.size(), synthParams.size());
	  logging.info(buffer);
	}

	if(picture < pictures.size()){ // shows a picture
	        if(images[picture] == NULL){
			SDL_Surface* image = IMG_Load(pictures[picture].c_str());
			
			if(image == NULL){
			  char buffer[120];
			  snprintf(buffer, 120, "showscreen: loading image FAILED (%s): %s",
				   SDL_GetError(), pictures[picture].c_str());
			  logging.warn(buffer);
			}
			
			SDL_Rect imageRect;
			SDL_Surface* scaled = 0;


			if(image != 0){
			        if((image->w) > (image->h)){
				  double wscale = ((double)SCREEN_WIDTH)/((double)image->w);
					// scaled = zoomSurface(image, wscale, wscale, SMOOTHING_ON);

					scaled = SDL_CreateRGBSurface(0, (int)(image->w*wscale), (int)(image->h*wscale), 32,
							0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

					if(SDL_BlitScaled(image, NULL, scaled, NULL) != 0)
					  return false;

				}
				else{
					double hscale = ((double)SCREEN_HEIGHT)/((double)image->h);
					// scaled = zoomSurface(image, hscale, hscale, SMOOTHING_ON);

					scaled = SDL_CreateRGBSurface(0, (int)(image->w*hscale), (int)(image->h*hscale), 32,
							0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

					if(SDL_BlitScaled(image, NULL, scaled, NULL) != 0)
					  return false;
				}

				images[picture] = scaled;
			}

			if(scaled != NULL){
			        SDL_Color averageColor;
				measureColor(scaled, averageColor);

				bgcolor = (int)(averageColor.r + averageColor.g + averageColor.b)/3;

				imageRect.w = scaled->w;
				imageRect.h = scaled->h;
				imageRect.x = (SCREEN_WIDTH - scaled->w)/2;
				imageRect.y = (SCREEN_HEIGHT - scaled->h)/2;

				if(SDL_BlitSurface(images[picture], NULL, surface, &imageRect) != 0)
				  return false;

				elementsDisplayed++;
			}

			if(image)
				SDL_FreeSurface(image);
		}
		else{
		        SDL_Rect imageRect;
			SDL_Surface* scaled = 0;

			scaled = images[picture];

			SDL_Color averageColor;

			measureColor(scaled, averageColor);

			bgcolor = (int)(averageColor.r + averageColor.g + averageColor.b)/3;

			imageRect.w = scaled->w;
			imageRect.h = scaled->h;
			imageRect.x = (SCREEN_WIDTH - scaled->w)/2;
			imageRect.y = (SCREEN_HEIGHT - scaled->h)/2;

			if(SDL_BlitSurface(images[picture], NULL, surface, &imageRect) != 0)
			  return false;

			elementsDisplayed++;
		}
	}


	///////////////////////////////////////////////////////////////////////
	// displays random curve (5 random points) [eye candy (for now)]

	
	if(showCurve && mic != NULL)
	{
	  std::vector< math::vertex< math::blas_real<double> > > curve;
	  std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > points;
	  const unsigned int NPOINTS = 5;
	  const unsigned int DIMENSION = 2; // 3

	  const double TICKSPERCURVE = CURVETIME/(TICK_MS/1000.0); // 0.5 second long buffer
	  curveParameter = (tick - latestTickCurveDrawn)/TICKSPERCURVE;
	  double stdev = 0.0;

	  // estimates sound dbel variance during latest CURVETIME and 
	  // scales curve noise/stddev according to distances from
	  // mean dbel value
	  {
	    // assumes we dont miss any ticks..
	    historyPower.push_back(mic->getInputPower());

	    while(historyPower.size() > TICKSPERCURVE/5.0) // 1.0 sec long history
	      historyPower.pop_front();

	    auto m = 0.0, v = 0.0;

	    for(auto h : historyPower){
	      m += h/historyPower.size();
	      v += h*h/historyPower.size();
	    }

	    v -= m*m;
	    v = sqrt(abs(v));

	    double t = mic->getInputPower();
	    stdev = 4.0;

	    double limit = m+v;
	    
	    if(t > limit){
	      stdev += 50.0*(abs(limit) + v)/abs(limit);
	    }
	    else{
	    }

	    // printf("MIC : %f %f %f\n", mic->getInputPower(), limit, stdev); fflush(stdout);

	  }
	  
	  
	  if(curveParameter > 1.0)
	  {
	    points.resize(NPOINTS);
	    
	    for(auto& p : points){
	      p.resize(DIMENSION);
	      
	      for(unsigned int d=0;d<DIMENSION;d++){
		whiteice::math::blas_real<float> value = rng.uniform()*2.0f - 1.0f; // [-1,1]
		p[d] = value.c[0];
	      }
	      
	    }

	    startPoint = endPoint;
	    endPoint = points;

	    if(startPoint.size() == 0)
	      startPoint = points;

	    latestTickCurveDrawn = tick;
	    curveParameter = (tick - latestTickCurveDrawn)/TICKSPERCURVE;
	  }
	  
	  {
	    points.resize(NPOINTS);
	    
	    for(unsigned int j=0;j<points.size();j++){
	      points[j].resize(DIMENSION);
	      for(unsigned int d=0;d<DIMENSION;d++){
		points[j][d] = (1.0 - curveParameter)*startPoint[j][d] + curveParameter*endPoint[j][d];
	      }
	      
	    }
	  }

	  // creates curve that goes through points
	  createHermiteCurve(curve, points, 0.001 + 0.01*stdev/4.0, 10000);

	  
	  for(const auto& p : curve){
	    int x = 0;
	    double scalingx = SCREEN_WIDTH/4;
	    math::convert(x, scalingx*p[0] + SCREEN_WIDTH/2);
	    int y = 0;
	    double scalingy = SCREEN_HEIGHT/4;
	    math::convert(y, scalingy*p[1] + SCREEN_HEIGHT/2);

	    if(x>=0 && x<SCREEN_WIDTH && y>=0 && y<SCREEN_HEIGHT){
	      Uint8 * pixel = (Uint8*)surface->pixels;
	      pixel += (y * surface->pitch) + (x * sizeof(Uint32));

	      if(bgcolor >= 160)
		*((Uint32*)pixel) = 0x00000000; // black
	      else
		*((Uint32*)pixel) = 0x00FFFFFF; // white
	    }
	    
	  }
 
	}

	
	///////////////////////////////////////////////////////////////////////
	// displays a text

	{
		SDL_Color white = { 255, 255, 255 };
//		SDL_Color red   = { 255, 0  , 0   };
//		SDL_Color green = { 0  , 255, 0   };
//		SDL_Color blue  = { 0  , 0  , 255 };
		SDL_Color black = { 0  , 0  , 0   };
		SDL_Color color;

		color = white;
		if(bgcolor > 160)
			color = black;
		
		if(font != NULL){
		  
		  SDL_Surface* msg = TTF_RenderUTF8_Blended(font, message.c_str(), color);
		  
		  if(msg != NULL)
		    elementsDisplayed++;
		  
		  SDL_Rect messageRect;
		  
		  messageRect.x = (SCREEN_WIDTH - msg->w)/2;
		  messageRect.y = (SCREEN_HEIGHT - msg->h)/2;
		  messageRect.w = msg->w;
		  messageRect.h = msg->h;
		  
		  if(SDL_BlitSurface(msg, NULL, surface, &messageRect) != 0)
		    return false;
		  
		  SDL_FreeSurface(msg);
		}

	}

	
	///////////////////////////////////////////////////////////////////////
	// video encoding (if activated)
	{
	  if(video && programStarted > 0){
	    auto t1 = std::chrono::system_clock::now().time_since_epoch();
	    auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

	    logging.info("adding frame to theora encoding queue");
	    
	    if(video->insertFrame((t1ms - programStarted), surface) == false)
	      logging.error("inserting frame FAILED");
	  }
	}


	///////////////////////////////////////////////////////////////////////
	// plays sound
	if(synth)
	{
	  // changes synth parameters only as fast sound synthesis can generate
	  // meaningful sounds (sound has time to evolve)
	  
	  auto t1 = std::chrono::system_clock::now().time_since_epoch();
	  auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
	  unsigned long long now = (unsigned long long)t1ms;
	  
	  if(now - synthParametersChangedTime >= MEASUREMODE_DELAY_MS){
	    synthParametersChangedTime = now;
	    
	    if(synth->setParameters(synthParams) == true){
	      elementsDisplayed++;
	    }
	    else
	      logging.warn("synth setParameters FAILED");
	  }
	}

	{
	  char buffer[256];
	  snprintf(buffer, 256, "engine_showScreen(%s %d/%d dim(%d)) = %d. DONE",
		   message.c_str(), picture, pictures.size(), synthParams.size(), elementsDisplayed);
	  logging.info(buffer);
	}

	return (elementsDisplayed > 0);
}


void ResonanzEngine::engine_pollEvents()
{
       SDL_Event event;

	while(SDL_PollEvent(&event)){
		// currently ignores all incoming events
		// (should handle window close event somehow)

		if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP){
			std::lock_guard<std::mutex> lock(keypress_mutex);
			keypressed = true;
		}
	}
}


void ResonanzEngine::engine_updateScreen()
{
  if(window != nullptr){
    if(SDL_UpdateWindowSurface(window) != 0){
      printf("engine_updateScreen() failed: %s\n", SDL_GetError());
    }
  }
}


// initializes SDL libraries to be used (graphics, font, music)
bool ResonanzEngine::engine_SDL_init(const std::string& fontname)
{
        logging.info("Starting SDL init (0)..");
        
	SDL_Init(0);

	logging.info("Starting SDL subsystem init (events, video, audio)..");

	if(SDL_InitSubSystem(SDL_INIT_EVENTS) != 0){
	  logging.error("SDL_Init(EVENTS) FAILED!");
	  return false;
	}
	else
	  logging.info("Starting SDL_Init(EVENTS) done..");
	
	if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0){
	  logging.error("SDL_Init(VIDEO) FAILED!");
	  return false;
	}
	else
	  logging.info("Starting SDL_Init(VIDEO) done..");

	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0){
	  logging.error("SDL_Init(AUDIO) FAILED!");
	  return false;
	}
	else
	  logging.info("Starting SDL_Init(AUDIO) done..");
	

	SDL_DisplayMode mode;

	if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
		SCREEN_WIDTH = mode.w;
		SCREEN_HEIGHT = mode.h;
	}

	logging.info("Starting SDL_GetCurrentDisplayMode() done..");

	if(TTF_Init() != 0){
		char buffer[80];
		snprintf(buffer, 80, "TTF_Init failed: %s\n", TTF_GetError());
		logging.error(buffer);
		throw std::runtime_error("TTF_Init() failed.");
	}

	logging.info("Starting TTF_Init() done..");

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;

	if(IMG_Init(flags) != flags){
		char buffer[80];
		snprintf(buffer, 80, "IMG_Init failed: %s\n", IMG_GetError());
		logging.error(buffer);
		IMG_Quit();
		throw std::runtime_error("IMG_Init() failed.");
	}

	logging.info("Starting IMG_Init() done..");

	flags = MIX_INIT_OGG;

	audioEnabled = false;

	if(0){
	  synth = new FMSoundSynthesis(); // curretly just supports single synthesizer type
	  mic   = new SDLMicListener();   // records single input channel
	  synth->pause(); // no sounds
	  
	  logging.info("Created sound synthesizer and capture objects..");
	}
	else{
	  synth = nullptr;
	  mic = nullptr;
	  
	  logging.info("Created sound synthesizer and capture DISABLED.");
	}
	

	if(mic != nullptr)
	  if(mic->listen() == false)      // tries to start recording audio
	    logging.error("starting SDL sound capture failed");
	
	{
	  // we get the mutex so eeg cannot change below us..
	  std::lock_guard<std::mutex> lock(eeg_mutex); 
	  
	  std::vector<unsigned int> nnArchitecture;
	  
	  if(synth){			  
	    nnArchitecture.push_back(eeg->getNumberOfSignals() + 
				     2*synth->getNumberOfParameters());
	    nnArchitecture.push_back(neuralnetwork_complexity*
				     (eeg->getNumberOfSignals() + 
				      2*synth->getNumberOfParameters()));
	    nnArchitecture.push_back(neuralnetwork_complexity*
				     (eeg->getNumberOfSignals() + 
				      2*synth->getNumberOfParameters()));
	    nnArchitecture.push_back(eeg->getNumberOfSignals());
	    
	    if(nnsynth != nullptr) delete nnsynth;
	    
	    nnsynth = new whiteice::nnetwork<>(nnArchitecture);
	    nnsynth->setNonlinearity(nnsynth->getLayers()-1,
				     whiteice::nnetwork<>::pureLinear);
	  }
	  else{
	    const int synth_number_of_parameters = 6;
	    
	    nnArchitecture.push_back(eeg->getNumberOfSignals() + 
				     2*synth_number_of_parameters);
	    nnArchitecture.push_back(neuralnetwork_complexity*
				     (eeg->getNumberOfSignals() + 
				      2*synth_number_of_parameters));
	    nnArchitecture.push_back(neuralnetwork_complexity*
				     (eeg->getNumberOfSignals() + 
				      2*synth_number_of_parameters));
	    nnArchitecture.push_back(eeg->getNumberOfSignals());
	    
	    if(nnsynth != nullptr) delete nnsynth;
	    
	    nnsynth = new whiteice::nnetwork<>(nnArchitecture);
	    nnsynth->setNonlinearity(nnsynth->getLayers()-1,
				     whiteice::nnetwork<>::pureLinear);
	  }
	}
	
	//#if 0
	if(Mix_Init(flags) != flags){
		char buffer[80];
		snprintf(buffer, 80, "Mix_Init failed: %s\n", Mix_GetError());
		logging.warn(buffer);
		audioEnabled = false;
		/*
		IMG_Quit();
		Mix_Quit();
		throw std::runtime_error("Mix_Init() failed.");
		*/
	}

	logging.info("Starting Mix_Init() done..");
	
	font = 0;

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1){
		audioEnabled = false;
		char buffer[80];
		snprintf(buffer, 80, "ERROR: Cannot open SDL mixer: %s.\n", Mix_GetError());
		logging.warn(buffer);
	}
	else audioEnabled = true;
	//#endif
	
	logging.info("SDL initialization.. SUCCESSFUL");
	
	return true;
}


bool ResonanzEngine::engine_SDL_deinit()
{
        logging.info("SDL deinitialization..");

	if(synth){
	  synth->pause();
	  delete synth;
	  synth = nullptr;	  
	}

	if(mic){
	  delete mic;
	  mic = nullptr;
	}

	if(audioEnabled)
	  SDL_CloseAudio();

	if(font){
	  TTF_CloseFont(font);
	  font = NULL;
	}
	
	IMG_Quit();

	if(audioEnabled)
		Mix_Quit();

	TTF_Quit();
	SDL_Quit();
	
	logging.info("SDL deinitialization.. DONE");

	return true;
}


void ResonanzEngine::engine_stopHibernation()
{
#ifdef _WIN32
	// resets hibernation timers
	SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
#endif
}



bool ResonanzEngine::engine_playAudioFile(const std::string& audioFile)
{
	if(audioEnabled){
		music = Mix_LoadMUS(audioFile.c_str());

		if(music != NULL){
			if(Mix_PlayMusic(music, -1) == -1){
				Mix_FreeMusic(music);
				music = NULL;
				logging.warn("sdl-music: cannot start playing music");
				return false;
			}
			return true;
		}
		else{
			char buffer[80];
			snprintf(buffer, 80, "sdl-music: loading audio file failed: %s", audioFile.c_str());
			logging.warn(buffer);
			return false;
		}
	}
	else return false;
}


bool ResonanzEngine::engine_stopAudioFile()
{
	if(audioEnabled){
		Mix_FadeOutMusic(50);

		if(music == NULL){
			return false;
		}
		else{
			Mix_FreeMusic(music);
			music = NULL;
		}

		return true;
	}
	else return false;
}


bool ResonanzEngine::measureColor(SDL_Surface* image, SDL_Color& averageColor)
{
	double r = 0.0;
	double g = 0.0;
	double b = 0.0;
	unsigned int N = 0;

	// SDL_Surface* im = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_Surface* im = image; // skip surface conversion/copy

	// if(im == 0) return false;

	unsigned int* buffer = (unsigned int*)im->pixels;

	// instead of calculating value for the whole image, we just calculate sample from 1000 pixels

	for(unsigned int s=0;s<1000;s++){
		const int x = rand() % im->w;
		const int y = rand() % im->h;

		int rr = (buffer[x + y*(image->pitch/4)] & 0xFF0000) >> 16;
		int gg = (buffer[x + y*(image->pitch/4)] & 0x00FF00) >> 8;
		int bb = (buffer[x + y*(image->pitch/4)] & 0x0000FF) >> 0;

		r += rr;
		g += gg;
		b += bb;

		N++;
	}

#if 0
	for(int y=0;y<(im->h);y++){
		for(int x=0;x<(im->w);x++){
			int rr = (buffer[x + y*(image->pitch/4)] & 0xFF0000) >> 16;
			int gg = (buffer[x + y*(image->pitch/4)] & 0x00FF00) >> 8;
			int bb = (buffer[x + y*(image->pitch/4)] & 0x0000FF) >> 0;

			r += rr;
			g += gg;
			b += bb;

			N++;
		}
	}
#endif

	r /= N;
	g /= N;
	b /= N;

	averageColor.r = (Uint8)r;
	averageColor.g = (Uint8)g;
	averageColor.b = (Uint8)b;

	// SDL_FreeSurface(im);

	return true;
}




bool ResonanzEngine::loadWords(const std::string filename, std::vector<std::string>& words) const
{
  FILE* handle = fopen(filename.c_str(), "rt");
  
  if(handle == 0)
    return false;
  
  char buffer[256];
  
  while(fgets(buffer, 256, handle) == buffer){
    const int N = strlen(buffer);
    
    for(int i=0;i<N;i++)
      if(buffer[i] == '\n' || buffer[i] == '\r')
	buffer[i] = '\0';
    
    if(strlen(buffer) > 1)
      words.push_back(buffer);
  }
  
  fclose(handle);
  
  return true;
}


bool ResonanzEngine::loadPictures(const std::string directory, std::vector<std::string>& pictures) const
{
	// looks for pics/*.jpg and pics/*.png files

	DIR* dp;

	struct dirent *ep;

	dp = opendir(directory.c_str());

	if(dp == 0) return false;

	while((ep = readdir(dp)) != NULL){
		if(ep->d_name == NULL)
			continue;

		std::string name = ep->d_name;

		if(name.find(".jpg") == (name.size() - 4) ||
			name.find(".png") == (name.size() - 4) ||
			name.find(".JPG") == (name.size() - 4) ||
			name.find(".PNG") == (name.size() - 4))
		{
			std::string fullname = directory.c_str();
			fullname = fullname + "/";
			fullname = fullname + name;

			pictures.push_back(fullname);
		}
	}

	closedir(dp);

	return true;
}


std::string ResonanzEngine::analyzeModel(const std::string& modelDir) const
{
	// we go through database directory and load all *.ds files
	std::vector<std::string> databaseFiles;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (modelDir.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			const char* filename = ent->d_name;

			if(strlen(filename) > 3)
				if(strcmp(&(filename[strlen(filename)-3]),".ds") == 0)
					databaseFiles.push_back(filename);
		}
		closedir (dir);
	}
	else { /* could not open directory */
		return "Cannot read directory";
	}

	unsigned int minDSSamples = (unsigned int)(-1);
	double avgDSSamples = 0;
	unsigned int N = 0;
	unsigned int failed = 0;
	unsigned int models = 0;

	float total_error = 0.0f;
	float total_N     = 0.0f;


	// std::lock_guard<std::mutex> lock(database_mutex);
	// (we do read only operations so these are relatively safe) => no mutex

	for(auto filename : databaseFiles){
		// calculate statistics
		whiteice::dataset<> ds;
		std::string fullname = modelDir + "/" + filename;
		if(ds.load(fullname) == false){
			failed++;
			continue; // couldn't load this dataset
		}

		if(ds.size(0) < minDSSamples) minDSSamples = ds.size(0);
		avgDSSamples += ds.size(0);
		N++;

		std::string modelFilename = fullname.substr(0, fullname.length()-3) + ".model";

		// check if there is a model file and load it into memory and TODO: calculate average error
		whiteice::bayesian_nnetwork<> nnet;

		if(nnet.load(modelFilename)){
			models++;

			if(ds.getNumberOfClusters() != 2)
				continue;

			if(ds.size(0) != ds.size(1))
				continue;

			float error = 0.0f;
			float error_N = 0.0f;

			for(unsigned int i=0;i<ds.size(0);i++){
				math::vertex<> m;
				math::matrix<> cov;

				auto x = ds.access(0, i);

				if(nnet.calculate(x, m, cov, 1, 0) == false)
					continue;

				auto y = ds.access(1, i);

				// converts data to real output values for meaningful comparision against cases WITHOUT preprocessing
				if(ds.invpreprocess(1, m) == false || ds.invpreprocess(1, y) == false)
					continue; // skip this datapoints

				auto delta = y - m;
				error += delta.norm().c[0];
				error_N++;
			}

			if(error_N > 0.0f){
				error /= error_N; // average error for this stimulation element
				total_error += error;
				total_N++;
			}
		}

	}

	if(total_N > 0.0f)
		total_error /= total_N;


	if(N > 0){
		avgDSSamples /= N;
		double modelPercentage = 100*models/((double)N);

		char buffer[1024];
		snprintf(buffer, 1024, "%d entries (%.0f%% has a model). samples(avg): %.2f, samples(min): %d\nAverage model error: %f\n",
				N, modelPercentage, avgDSSamples, minDSSamples, total_error);

		return buffer;
	}
	else{
		char buffer[1024];
		snprintf(buffer, 1024, "%d entries (0%% has a model). samples(avg): %.2f, samples(min): %d",
				0, 0.0, 0);

		return buffer;
	}
}


// analyzes given measurements database and model performance more accurately
std::string ResonanzEngine::analyzeModel2(const std::string& pictureDir, 
					  const std::string& keywordsFile, 
					  const std::string& modelDir) const
{
  // 1. loads picture and keywords filename information into local memory
  std::vector<std::string> pictureFiles;
  std::vector<std::string> keywords;
  
  if(loadWords(keywordsFile, keywords) == false || 
     loadPictures(pictureDir, pictureFiles) == false)
    return "";
  
  // 2. loads dataset files (.ds) one by one if possible and calculates prediction error
  
  std::string report = "MODEL PREDICTION ERRORS:\n\n";
  
  // loads databases into memory
  for(unsigned int i=0;i<keywords.size();i++){
    std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";
    std::string modelFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".model";
    
    whiteice::dataset<> data;
    whiteice::bayesian_nnetwork<> bnn;
    
    if(data.load(dbFilename) == true && bnn.load(modelFilename)){
      if(data.getNumberOfClusters() == 2){
	// calculates average error
	float error = 0.0f;
	float num   = 0.0f;
	
	for(unsigned int j=0;j<data.size(0);j++){
	  auto input = data.access(0, j);
	  
	  math::vertex<> m;
	  math::matrix<> C;
	  
	  if(bnn.calculate(input, m, C, 1, 0)){
	    auto output = data.access(1, j);
	    
	    // we must inv-postprocess data before calculation error
	    
	    data.invpreprocess(1, m);
	    data.invpreprocess(1, output);
	    
	    auto delta = output - m;
	    
	    error += delta.norm().c[0];
	    num++;
	  }
	}
	  
	error /= num;
	
	char buffer[256];
	snprintf(buffer, 256, "Keyword '%s' model error: %f (N=%d)\n", 
		 keywords[i].c_str(), error, (int)num);
	
	report += buffer;
      }
    }
  }
  
  report += "\n";
  
  for(unsigned int i=0;i<pictureFiles.size();i++){
    std::string dbFilename = 
      modelDir + "/" + calculateHashName(pictureFiles[i] + eeg->getDataSourceName()) + ".ds";
    std::string modelFilename = 
      modelDir + "/" + calculateHashName(pictureFiles[i] + eeg->getDataSourceName()) + ".model";
    
    whiteice::dataset<> data;
    whiteice::bayesian_nnetwork<> bnn;
    
    if(data.load(dbFilename) == true && bnn.load(modelFilename)){
      if(data.getNumberOfClusters() == 2){
	// calculates average error
	float error = 0.0f;
	float num   = 0.0f;
	
	for(unsigned int j=0;j<data.size(0);j++){
	  auto input = data.access(0, j);
	  
	  math::vertex<> m;
	  math::matrix<> C;
	  
	  if(bnn.calculate(input, m, C, 1, 0)){
	    auto output = data.access(1, j);
	    
	    // we must inv-postprocess data before calculation error
	    
	    data.invpreprocess(1, m);
	    data.invpreprocess(1, output);
	    
	    auto delta = output - m;
	    
	    error += delta.norm().c[0];
	    num++;
	  }
	}
	
	error /= num;
	
	char buffer[256];
	snprintf(buffer, 256, "Picture '%s' model error: %f (N=%d)\n", 
		 pictureFiles[i].c_str(), error, (int)num);
	
	report += buffer;
      }
    }
  }
  
  report = report + "\n";
  
  unsigned int synth_N = 0;
  
  if(synth){
    std::string dbFilename = modelDir + "/" + 
      calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".ds";
    std::string modelFilename = modelDir + "/" + 
      calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".model";
    
    whiteice::dataset<> data;
    whiteice::bayesian_nnetwork<> bnn;    
    
    if(data.load(dbFilename) == true && bnn.load(modelFilename)){
      if(data.getNumberOfClusters() == 2){
	// calculates average error
	float error = 0.0f;
	float num   = 0.0f;
	
	for(unsigned int j=0;j<data.size(0);j++){
	  auto input = data.access(0, j);
	  
	  math::vertex<> m;
	  math::matrix<> C;
	  
	  if(bnn.calculate(input, m, C, 1, 0)){
	    auto output = data.access(1, j);
	    
	    // we must inv-postprocess data before calculation error
	    
	    data.invpreprocess(1, m);
	    data.invpreprocess(1, output);
	    
	    auto delta = output - m;
	    
	    error += delta.norm().c[0];
	    num++;
	  }
	}
	
	error /= num;
	
	char buffer[256];
	snprintf(buffer, 256, "Synth %s model [dim(%d) -> dim(%d)] error: %f (N=%d)\n", 
		 synth->getSynthesizerName().c_str(), 
		 bnn.inputSize(), bnn.outputSize(), 
		 error, (int)num);
	
	report += buffer;	
      }
    }
  }

  return report;
}


// measured program functions
bool ResonanzEngine::invalidateMeasuredProgram()
{
	// invalidates currently measured program
	std::lock_guard<std::mutex> lock(measure_program_mutex);

	this->measuredProgram.resize(0);

	return true;
}


bool ResonanzEngine::getMeasuredProgram(std::vector< std::vector<float> >& program)
{
	// gets currently measured program
	std::lock_guard<std::mutex> lock(measure_program_mutex);

	if(measuredProgram.size() == 0)
		return false;

	program = this->measuredProgram;

	return true;
}


// calculates delta statistics from the measurements [with currently selected EEG]
std::string ResonanzEngine::deltaStatistics(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir) const
{
	// 1. loads picture and keywords files into local memory
	std::vector<std::string> pictureFiles;
	std::vector<std::string> keywords;

	if(loadWords(keywordsFile, keywords) == false || loadPictures(pictureDir, pictureFiles) == false)
		return "";

	std::multimap<float, std::string> keywordDeltas;
	std::multimap<float, std::string> pictureDeltas;
	float mean_delta_keywords = 0.0f;
	float var_delta_keywords  = 0.0f;
	float mean_delta_pictures = 0.0f;
	float var_delta_pictures  = 0.0f;
	float mean_delta_synth    = 0.0f;
	float var_delta_synth     = 0.0f;
	float num_keywords = 0.0f;
	float num_pictures = 0.0f;
	float pca_preprocess = 0.0f;

	unsigned int input_dimension = 0;
	unsigned int output_dimension = 0;

	// 2. loads dataset files (.ds) one by one if possible and calculates mean delta
	whiteice::dataset<> data;

	// loads databases into memory or initializes new ones
	for(unsigned int i=0;i<keywords.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";

		data.clear();

		if(data.load(dbFilename) == true){
			if(data.getNumberOfClusters() == 2){
				float delta = 0.0f;

				for(unsigned int j=0;j<data.size(0);j++){
					auto d = data.access(1, j);
					delta += d.norm().c[0] / data.size(0);
				}

				if(data.size(0) > 0){
					input_dimension  = data.access(0, 0).size();
					output_dimension = data.access(1, 0).size();
				}

				std::pair<float, std::string> p;
				p.first  = -delta;
				
				char buffer[128];
				snprintf(buffer, 128, "%s (N = %d)", 
					 keywords[i].c_str(), data.size(0));
				std::string msg = buffer;
				
				p.second = msg;
				keywordDeltas.insert(p);

				mean_delta_keywords += delta;
				var_delta_keywords  += delta*delta;
				num_keywords++;

				if(data.hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval))
					pca_preprocess++;
			}
		}
	}

	mean_delta_keywords /= num_keywords;
	var_delta_keywords  /= num_keywords;
	var_delta_keywords  -= mean_delta_keywords*mean_delta_keywords;
	var_delta_keywords  *= num_keywords/(num_keywords - 1.0f);

	for(unsigned int i=0;i<pictureFiles.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(pictureFiles[i] + eeg->getDataSourceName()) + ".ds";

		data.clear();

		if(data.load(dbFilename) == true){
			if(data.getNumberOfClusters() == 2){
				float delta = 0.0f;

				for(unsigned int j=0;j<data.size(0);j++){
					auto d = data.access(1, j);
					delta += d.norm().c[0] / data.size(0);
				}

				if(data.size(0) > 0){
					input_dimension  = data.access(0, 0).size();
					output_dimension = data.access(1, 0).size();
				}

				std::pair<float, std::string> p;
				p.first  = -delta;

				char buffer[128];
				snprintf(buffer, 128, "%s (N = %d)", 
					 pictureFiles[i].c_str(), data.size(0));
				std::string msg = buffer;
				
				p.second = msg;
				pictureDeltas.insert(p);

				mean_delta_pictures += delta;
				var_delta_pictures  += delta*delta;
				num_pictures++;

				if(data.hasPreprocess(0, whiteice::dataset<>::dnCorrelationRemoval))
					pca_preprocess++;
			}
		}
	}

	mean_delta_pictures /= num_pictures;
	var_delta_pictures  /= num_pictures;
	var_delta_pictures  -= mean_delta_pictures*mean_delta_pictures;
	var_delta_pictures  *= num_pictures/(num_pictures - 1.0f);
	

	unsigned int synth_N = 0;
	
	if(synth){
	  std::string dbFilename = modelDir + "/" + 
	    calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".ds";
	  
	  data.clear();

	  if(data.load(dbFilename) == true){
	    if(data.getNumberOfClusters() == 2){
	      
	      float delta = 0.0f;

	      for(unsigned int j=0;j<data.size(0);j++){
		auto d = data.access(1, j);
		delta += d.norm().c[0] / data.size(0);
	      }
	      
	      synth_N = data.size(0);
	      
	      mean_delta_synth += delta;
	      var_delta_synth  += delta*delta;
	    }
	  }
	  
	}
	
	var_delta_synth -= mean_delta_synth*mean_delta_synth;

	// 3. sorts deltas/keyword delta/picture (use <multimap> for automatic ordering) and prints the results

	std::string report = "";
	const unsigned int BUFSIZE = 512;
	char buffer[BUFSIZE];

	snprintf(buffer, BUFSIZE, "Picture delta: %.2f stdev(delta): %.2f\n", mean_delta_pictures, sqrt(var_delta_pictures));
	report += buffer;
	
	if(keywords.size() > 0){
		snprintf(buffer, BUFSIZE, "Keyword delta: %.2f stdev(delta): %.2f\n", mean_delta_keywords, sqrt(var_delta_keywords));
		report += buffer;
	}
	
	snprintf(buffer, BUFSIZE, "Synth delta: %.2f stdev(delta): %.2f (N = %d)\n", mean_delta_synth, sqrt(var_delta_synth), synth_N);
	report += buffer;
	
	snprintf(buffer, BUFSIZE, "PCA preprocessing: %.1f%% of elements\n", 100.0f*pca_preprocess/(num_pictures + num_keywords));
	report += buffer;
	snprintf(buffer, BUFSIZE, "Input dimension: %d Output dimension: %d\n", input_dimension, output_dimension);
	report += buffer;
	snprintf(buffer, BUFSIZE, "\n");
	report += buffer;

	snprintf(buffer, BUFSIZE, "PICTURE DELTAS\n");
	report += buffer;
	for(auto& a : pictureDeltas){
		snprintf(buffer, BUFSIZE, "%s: delta %.2f\n", a.second.c_str(), -a.first);
		report += buffer;
	}
	snprintf(buffer, BUFSIZE, "\n");
	report += buffer;

	snprintf(buffer, BUFSIZE, "KEYWORD DELTAS\n");
	report += buffer;
	for(auto& a : keywordDeltas){
		snprintf(buffer, BUFSIZE, "%s: delta %.2f\n", a.second.c_str(), -a.first);
		report += buffer;
	}
	snprintf(buffer, BUFSIZE, "\n");
	report += buffer;

	return report;
}


// returns collected program performance statistics [program weighted RMS]
std::string ResonanzEngine::executedProgramStatistics() const
{
	if(programRMS_N > 0){
		float rms = programRMS / programRMS_N;

		char buffer[80];
		snprintf(buffer, 80, "Program performance (average error): %.4f.\n", rms);
		std::string result = buffer;

		return result;
	}
	else{
		return "No program performance data available.\n";
	}
}


// exports data to ASCII format files (.txt files)
bool ResonanzEngine::exportDataAscii(const std::string& pictureDir, 
				     const std::string& keywordsFile, 
				     const std::string& modelDir) const
{
	// 1. loads picture and keywords files into local memory
	std::vector<std::string> pictureFiles;
	std::vector<std::string> keywords;

	if(loadWords(keywordsFile, keywords) == false || 
	   loadPictures(pictureDir, pictureFiles) == false)
	  return false;
	
	// 2. loads dataset files (.ds) one by one if possible and calculates mean delta
	whiteice::dataset<> data;

	// loads databases into memory or initializes new ones
	for(unsigned int i=0;i<keywords.size();i++){
		std::string dbFilename = modelDir + "/" + 
		  calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";
		std::string txtFilename = modelDir + "/" + "KEYWORD_" + 
		  keywords[i] + "_" + eeg->getDataSourceName() + ".txt";
		
		data.clear();

		if(data.load(dbFilename) == true){
		  if(data.exportAscii(txtFilename) == false)
		    return false;
		}
		else return false;
	}

	for(unsigned int i=0;i<pictureFiles.size();i++){
	  std::string dbFilename = modelDir + "/" + 
	    calculateHashName(pictureFiles[i] + eeg->getDataSourceName()) + ".ds";
	  
	  char filename[2048];
	  snprintf(filename, 2048, "%s", pictureFiles[i].c_str());
	  
	  std::string txtFilename = modelDir + "/" + "PICTURE_" + 
	    basename(filename) + "_" + eeg->getDataSourceName() + ".txt";
	  
	  data.clear();
	  
	  if(data.load(dbFilename) == true){
	    if(data.exportAscii(txtFilename) == false)
	      return false;
	  }
	  else return false;
	}

	
	if(synth){
	  std::string dbFilename = modelDir + "/" + 
	    calculateHashName(eeg->getDataSourceName() + synth->getSynthesizerName()) + ".ds";
	  
	  std::string sname = synth->getSynthesizerName();
	  
	  char synthname[2048];
	  snprintf(synthname, 2048, "%s", sname.c_str());
	  
	  for(unsigned int i=0;i<strlen(synthname);i++)
	    if(isalnum(synthname[i]) == 0) synthname[i] = '_';
	  
	  std::string txtFilename = modelDir + "/" + "SYNTH_" +  
	    synthname + "_" + eeg->getDataSourceName() + ".txt";
	  
	  data.clear();
	  
	  if(data.load(dbFilename) == true){
	    // export data fails here for some reason => figure out why (dump seems to be ok)..
	    if(data.exportAscii(txtFilename) == false){
	      return false;
	    }
	  }
	  else{
	    return false;
	  }
	}
	
	return true;
}





bool ResonanzEngine::deleteModelData(const std::string& modelDir)
{
	// we go through database directory and delete all *.ds and *.model files
	std::vector<std::string> databaseFiles;
	std::vector<std::string> modelFiles;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (modelDir.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			const char* filename = ent->d_name;

			if(strlen(filename) > 3)
				if(strcmp(&(filename[strlen(filename)-3]),".ds") == 0)
					databaseFiles.push_back(filename);
		}
		closedir (dir);
	}
	else { /* could not open directory */
		return false;
	}

	dir = NULL;
	ent = NULL;
	if ((dir = opendir (modelDir.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			const char* filename = ent->d_name;

			if(strlen(filename) > 6)
				if(strcmp(&(filename[strlen(filename)-6]),".model") == 0)
					modelFiles.push_back(filename);
		}
		closedir (dir);
	}
	else { /* could not open directory */
		return false;
	}

	logging.info("about to delete models and measurements database..");

	// prevents access to database from other threads
	std::lock_guard<std::mutex> lock1(database_mutex);
	// operation locks so that other command cannot start
	std::lock_guard<std::mutex> lock2(command_mutex);

	if(currentCommand.command != ResonanzCommand::CMD_DO_NOTHING)
		return false;

	if(keywordData.size() > 0 || pictureData.size() > 0 ||
		keywordModels.size() > 0 || pictureModels.size() > 0)
		return false; // do not delete anything if there models/data is loaded into memory

	for(auto filename : databaseFiles){
		auto f = modelDir + "/" + filename;
		remove(f.c_str());
	}

	for(auto filename : modelFiles){
		auto f = modelDir + "/" + filename;
		remove(f.c_str());
	}

	logging.info("models and measurements database deleted");

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ResonanzCommand::ResonanzCommand(){
	this->command = ResonanzCommand::CMD_DO_NOTHING;
	this->showScreen = false;
	this->pictureDir = "";
	this->keywordsFile = "";
	this->modelDir = "";
}

ResonanzCommand::~ResonanzCommand(){

}


} /* namespace resonanz */
} /* namespace whiteice */
