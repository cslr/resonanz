/*
 * ResonanzEngine.cpp
 *
 *  Created on: 13.6.2015
 *      Author: Tomas
 */

#include "ResonanzEngine.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <map>

#include <math.h>
#include <dirent.h>

#include <dinrhiw/dinrhiw.h>

#include "Log.h"
#include "NMCFile.h"

#include "NoEEGDevice.h"
#include "RandomEEG.h"
#include "EmotivInsightStub.h"
#include "EmotivInsightPipeServer.h"
#include "LightstoneDevice.h"

#include "SDLTheora.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace whiteice {
namespace resonanz {


ResonanzEngine::ResonanzEngine()
{
	std::lock_guard<std::mutex> lock(thread_mutex);

	engine_setStatus("resonanz-engine: starting..");

	workerThread = nullptr;
	thread_is_running = true;

	{
		std::lock_guard<std::mutex> lock(command_mutex);
		incomingCommand = nullptr;
		currentCommand.command = ResonanzCommand::CMD_DO_NOTHING;
		currentCommand.showScreen = false;
	}

	eeg = nullptr;

	// starts updater thread thread
	workerThread = new std::thread(std::bind(ResonanzEngine::engine_loop, this));
	workerThread->detach();
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
		workerThread = new std::thread(std::bind(ResonanzEngine::engine_loop, this));
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


bool ResonanzEngine::cmdRandom(const std::string& pictureDir, const std::string& keywordsFile) throw()
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

	for(auto& p : program)
		NMCFile::interpolateProgram(p);

	incomingCommand->command = ResonanzCommand::CMD_DO_EXECUTE;
	incomingCommand->showScreen = true;
	incomingCommand->pictureDir = pictureDir;
	incomingCommand->keywordsFile = keywordsFile;
	incomingCommand->modelDir = modelDir;
	incomingCommand->audioFile = audioFile;
	incomingCommand->signalName = targetSignal;
	incomingCommand->programValues = program;
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
	try{
		std::lock_guard<std::mutex> lock1(eeg_mutex);

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
		else if(deviceNumber == ResonanzEngine::RE_EEG_EMOTIV_INSIGHT_DEVICE){
			if(eeg != nullptr) delete eeg;
			eeg = new EmotivInsightPipeServer("\\\\.\\pipe\\emotiv-insight-data");
		}
		else if(deviceNumber == ResonanzEngine::RE_EEG_IA_MUSE_DEVICE){
			return false; // not currently supported
		}
		else if(deviceNumber == ResonanzEngine::RE_WD_LIGHTSTONE){
			if(eeg != nullptr) delete eeg;
			eeg = new LightstoneDevice();
		}
		else{
			return false; // unknown device
		}

		// updates neural network model according to signal number of the EEG device
		{
			std::vector<unsigned int> nnArchitecture;
			nnArchitecture.push_back(eeg->getNumberOfSignals());
			nnArchitecture.push_back(4*eeg->getNumberOfSignals());
			nnArchitecture.push_back(4*eeg->getNumberOfSignals());
			nnArchitecture.push_back(eeg->getNumberOfSignals());

			if(nn != nullptr) delete nn;

			nn = new whiteice::nnetwork<>(nnArchitecture);
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
					sprintf(buffer, "%.2f ", values[i]);
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
	long long tick = 0;


	// later autodetected to good values based on display screen resolution
	SCREEN_WIDTH  = 800;
	SCREEN_HEIGHT = 600;

	const std::string fontname = "Vera.ttf";

	{
		std::lock_guard<std::mutex> lock(eeg_mutex);
		eeg = new NoEEGDevice();
		eegDeviceType = ResonanzEngine::RE_EEG_NO_DEVICE;
	}

	std::vector<unsigned int> nnArchitecture;
	nnArchitecture.push_back(eeg->getNumberOfSignals());
	nnArchitecture.push_back(4*eeg->getNumberOfSignals());
	nnArchitecture.push_back(4*eeg->getNumberOfSignals());
	nnArchitecture.push_back(eeg->getNumberOfSignals());

	nn = new whiteice::nnetwork<>(nnArchitecture);


	unsigned int currentPictureModel = 0;
	unsigned int currentKeywordModel = 0;

	// used to execute program [targetting target values]
	const float programHz = 1.0; // 1 program step means 1 second
	std::vector< std::vector<float> > program;
	std::vector< std::vector<float> > programVar;
	programStarted = 0LL; // program has not been started
	long long lastProgramSecond = 0LL;



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
				engine_sleep(TICK_MS/5);
		}

		lastTickProcessed = tick;



		ResonanzCommand prevCommand = currentCommand;
		if(engine_checkIncomingCommand() == true){
			logging.info("new engine command received");
			// we must make engine state transitions, state transfer from the previous command to the new command

			// state exit actions:
			if(prevCommand.command == ResonanzCommand::CMD_DO_MEASURE){
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
				// removes unnecessarily data structures from memory (measurements database) [no need to save it because it was not changed]
				keywordData.clear();
				pictureData.clear();

				// also stops computation if needed
				if(optimizer != nullptr){
					if(optimizer->isRunning())
						optimizer->stopComputation();
					delete optimizer;
					optimizer = nullptr;
				}
			}
			else if(prevCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
				keywordData.clear();
				pictureData.clear();
				keywordModels.clear();
				pictureModels.clear();

				if(prevCommand.audioFile.length() > 0)
					engine_stopAudioFile();

				if(mcsamples.size() > 0)
					mcsamples.clear();

				// stops encoding if needed
				if(video != nullptr){
					auto t1 = std::chrono::system_clock::now().time_since_epoch();
					auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

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

				window = SDL_CreateWindow(windowTitle.c_str(),
							    SDL_WINDOWPOS_CENTERED,
							    SDL_WINDOWPOS_CENTERED,
							    SCREEN_WIDTH, SCREEN_HEIGHT,
							    SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);

				if(window != nullptr){
					SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

					SDL_Surface* icon = IMG_Load(iconFile.c_str());
					if(icon != nullptr){
						SDL_SetWindowIcon(window, icon);
						SDL_FreeSurface(icon);
					}

					SDL_Surface* surface = SDL_GetWindowSurface(window);
					SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
					SDL_RaiseWindow(window);
					SDL_SetWindowGrab(window, SDL_TRUE);
					SDL_UpdateWindowSurface(window);
					SDL_RaiseWindow(window);
					SDL_SetWindowGrab(window, SDL_FALSE);
				}

			}
			else if(currentCommand.showScreen == true && prevCommand.showScreen == true){
				// just empties current window with blank (black) screen
				if(window == nullptr){
					window = SDL_CreateWindow(windowTitle.c_str(),
								    SDL_WINDOWPOS_CENTERED,
								    SDL_WINDOWPOS_CENTERED,
								    SCREEN_WIDTH, SCREEN_HEIGHT,
								    SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
				}

				if(window != nullptr){
					SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

					SDL_Surface* icon = IMG_Load(iconFile.c_str());
					if(icon != nullptr){
						SDL_SetWindowIcon(window, icon);
						SDL_FreeSurface(icon);
					}

					SDL_Surface* surface = SDL_GetWindowSurface(window);
					SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
					SDL_RaiseWindow(window);
					SDL_SetWindowGrab(window, SDL_TRUE);
					SDL_UpdateWindowSurface(window);
					SDL_RaiseWindow(window);
					SDL_SetWindowGrab(window, SDL_FALSE);
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

				if(engine_loadMedia(currentCommand.pictureDir, currentCommand.keywordsFile, loadData) == false)
					logging.error("loading media files failed");
				else
					logging.info("loading media files successful");
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

				// checks there is enough data to do meaningful optimization
				bool aborted = false;

				for(unsigned int i=0;i<pictureData.size() && !aborted;i++){
					if(pictureData[i].size() < 10){
						engine_setStatus("resonanz-engine: less than 10 data points per picture/keyword => aborting optimization");
						logging.warn("aborting model optimization command because of too little data (less than 10 samples per case)");
						cmdDoNothing(false);
						aborted = true;
						break;
					}
				}

				for(unsigned int i=0;i<keywordData.size() && !aborted;i++){
					if(keywordData[i].size() < 10){
						engine_setStatus("resonanz-engine: less than 10 data points per picture/keyword => aborting optimization");
						logging.warn("aborting model optimization command because of too little data (less than 10 samples per case)");
						cmdDoNothing(false);
						aborted = true;
						break;
					}
				}

				if(aborted)
					continue; // do not start executing any commands [recheck command input buffer and move back to do nothing command]

			}

			if(currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
				try{
					engine_setStatus("resonanz-engine: loading prediction model..");

					if(engine_loadModels(currentCommand.modelDir) == false){
						logging.error("Couldn't load models from model dir: " + currentCommand.modelDir);
						this->cmdStopCommand();
						continue; // aborts initializing execute command
					}

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
							p[i] = 100000.0f; // 100.000 very large value (near infinite) => can take any value
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


					// initializes blind monte carlo data structures
					if(currentCommand.blindMonteCarlo){
						mcsamples.clear();

						for(unsigned int i=0;i<MONTE_CARLO_SIZE;i++){
							math::vertex<> u(names.size());
							for(unsigned int j=0;j<u.size();j++)
								u[j] = rand()/((double)RAND_MAX); // [0,1] valued signals sampled from [0,1]^D
							mcsamples.push_back(u);
						}
					}

					if(currentCommand.saveVideo){
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

					if(currentCommand.audioFile.length() > 0)
						engine_playAudioFile(currentCommand.audioFile);

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

				// currently just plays audio and shows blank screen
				if(currentCommand.audioFile.length() > 0)
					engine_playAudioFile(currentCommand.audioFile);

				// => ready to measure
			}

		}

		// executes current command
		if(currentCommand.command == ResonanzCommand::CMD_DO_NOTHING){
			engine_setStatus("resonanz-engine: sleeping..");

			engine_pollEvents(); // polls for events
			engine_updateScreen(); // always updates window if it exists
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM){
			engine_setStatus("resonanz-engine: showing random examples..");

			engine_stopHibernation();

			if(keywords.size() > 0 && pictures.size() > 0){
				unsigned int key = rand() % keywords.size();
				unsigned int pic = rand() % pictures.size();

				engine_showScreen(keywords[key], pic);
			}

			engine_pollEvents(); // polls for events
			engine_updateScreen(); // always updates window if it exists
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_MEASURE){
			engine_setStatus("resonanz-engine: measuring eeg-responses..");

			if(eeg->connectionOk() == false){
				logging.info("measure command: eeg connection failed => aborting measurements");

				cmdDoNothing(false); // new command: stops and starts idling

				engine_pollEvents(); // polls for events
				engine_updateScreen(); // always updates window if it exists

				continue;
			}

			engine_stopHibernation();

			if(keywords.size() > 0 && pictures.size() > 0){
				unsigned int key = rand() % keywords.size();
				unsigned int pic = rand() % pictures.size();

				engine_showScreen(keywords[key], pic);

				engine_pollEvents(); // polls for events
				engine_updateScreen(); // always updates window if it exists

				std::vector<float> eegBefore;
				std::vector<float> eegAfter;

				eeg->data(eegBefore);
				engine_sleep(MEASUREMODE_DELAY_MS);
				eeg->data(eegAfter);

				engine_pollEvents();

				engine_storeMeasurement(pic, key, eegBefore, eegAfter);
			}
			else{
				engine_pollEvents(); // polls for events
				engine_updateScreen(); // always updates window if it exists
			}
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_OPTIMIZE){
			const float percentage = (currentPictureModel + currentKeywordModel)/((float)(pictureData.size()+keywordData.size()));
			{
				char buffer[80];
				sprintf(buffer, "resonanz-engine: optimizing prediction model (%.2f%%)..", 100.0f*percentage);

				engine_setStatus(buffer);
			}

			engine_stopHibernation();

			if(engine_optimizeModels(currentPictureModel, currentKeywordModel) == false)
				logging.warn("model optimization failure");

		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_EXECUTE){
			engine_setStatus("resonanz-engine: executing program..");

			engine_stopHibernation();

			auto t1 = std::chrono::system_clock::now().time_since_epoch();
			auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

			long long currentSecond = (long long)
					(programHz*(t1ms - programStarted)/1000.0f); // gets current second for the program value

			if(currentSecond <= lastProgramSecond)
				continue; // nothing to do

			if(lastProgramSecond > 0){
				// calculates RMS error
				std::vector<float> current;
				std::vector<float> target;

				target.resize(program.size());
				for(unsigned int i=0;i<program.size();i++){
					// what was our target BEFORE this time tick [did we move to target state]
					target[i] = program[i][lastProgramSecond];
				}

				eeg->data(current);

				if(target.size() == current.size()){
					float rms = 0.0f;
					for(unsigned int i=0;i<target.size();i++){
						rms += (current[i] - target[i])*(current[i] - target[i]);
					}
					rms = sqrt(rms);

					// adds current rms to the global RMS
					programRMS += rms;
					programRMS_N++;
				}

			}

			lastProgramSecond = currentSecond;


			if(currentSecond < (signed)program[0].size()){
				// executes program
				std::vector<float> eegCurrent;
				std::vector<float> eegTarget;
				std::vector<float> eegTargetVariance;

				eeg->data(eegCurrent);

				eegTarget.resize(eegCurrent.size());
				eegTargetVariance.resize(eegCurrent.size());

				for(unsigned int i=0;i<eegTarget.size();i++){
					eegTarget[i] = program[i][currentSecond];
					eegTargetVariance[i] = programVar[i][currentSecond];
				}

				// shows picture/keyword which model predicts to give closest match to target
				// minimize(picture) ||f(picture,eegCurrent) - eegTarget||/eegTargetVariance

				if(currentCommand.blindMonteCarlo == false)
					engine_executeProgram(eegCurrent, eegTarget, eegTargetVariance);
				else
					engine_executeProgramMonteCarlo(eegTarget, eegTargetVariance);
			}
			else{

				// program has run to the end => stop
				logging.info("Executing the given program has stopped [program stop time].");

				if(video){
					auto t1 = std::chrono::system_clock::now().time_since_epoch();
					auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

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

			lastProgramSecond = currentSecond;

			if(currentSecond < currentCommand.programLengthTicks){
				// measurement program continues: just measures values and do nothing
				// as the background thread currently handles playing of the music
				// LATER: do video decoding and showing..

				std::vector<float> values(eeg->getNumberOfSignals());
				eeg->data(values);

				for(unsigned int i=0;i<rawMeasuredSignals.size();i++)
					rawMeasuredSignals[i].push_back(values[i]);

				engine_updateScreen();
				engine_pollEvents();
			}
			else{
				// stops measuring program:
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
							if(rawMeasuredSignals[n].size() < MIN)
								MIN = rawMeasuredSignals[n].size();
							for(unsigned int i=0;i<MIN;i++){
								if(rawMeasuredSignals[n][i] >= 0.0f){
									measuredProgram[j][i] = rawMeasuredSignals[n][i];
								}
							}
						}
					}
				}

				cmdStopCommand();
			}
		}

		engine_pollEvents();

		if(keypress()){
			if(currentCommand.command != ResonanzCommand::CMD_DO_NOTHING){
				logging.info("Received keypress: stopping command..");
				cmdStopCommand();
			}
		}

		// monitors current emotiv eeg values and logs them into log file
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
					sprintf(buffer, " %.2f", x[i]);
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

	engine_SDL_deinit();

}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// loads prediction models for program execution, returns false in case of failure
bool ResonanzEngine::engine_loadModels(const std::string& modelDir)
{
	if(keywords.size() <= 0 || pictures.size() <= 0)
		return false; // no loaded keywords of pictures

	pictureModels.resize(pictures.size());

	unsigned int pictureModelsLoaded = 0;
	unsigned int keywordModelsLoaded = 0;

	for(unsigned int i=0;i<pictureModels.size();i++){
		std::string filename = calculateHashName(pictures[i] + eeg->getDataSourceName()) + ".model";
		filename = modelDir + "/" + filename;

		if(pictureModels[i].load(filename) == false){
			logging.error("Loading model file failed: " + filename);
			continue;
		}

		pictureModelsLoaded++;
	}

	keywordModels.resize(keywords.size());

	for(unsigned int i=0;i<keywordModels.size();i++){
		std::string filename = calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".model";
		filename = modelDir + "/" + filename;

		if(keywordModels[i].load(filename) == false){
			logging.error("Loading model file failed: " + filename);
			continue;
		}

		keywordModelsLoaded++;
	}

	// returns true if could load at least one model for keywords and models
	return (pictureModelsLoaded > 0 && keywordModelsLoaded > 0);
}


// shows picture/keyword which model predicts to give closest match to target
// minimize(picture) ||f(picture,eegCurrent) - eegTarget||/eegTargetVariance
bool ResonanzEngine::engine_executeProgram(const std::vector<float>& eegCurrent,
		const std::vector<float>& eegTarget, const std::vector<float>& eegTargetVariance)
{
	int bestKeyword = -1;
	int bestPicture = -1;
	float bestError = std::numeric_limits<double>::infinity();

	math::vertex<> target(eegTarget.size());
	math::vertex<> targetVariance(eegTargetVariance.size());

	for(unsigned int i=0;i<target.size();i++){
		target[i] = eegTarget[i];
		targetVariance[i] = eegTargetVariance[i];
	}


	for(unsigned int index=0;index<keywordModels.size();index++){
		whiteice::nnetwork<>& model = keywordModels[index];

		if(model.input_size() != eegCurrent.size() || model.output_size() != eegTarget.size()){
			logging.warn("skipping bad keyword prediction model");
			continue; // bad model/data => ignore
		}

		math::vertex<> x(eegCurrent.size());
		for(unsigned int i=0;i<x.size();i++)
			x[i] = eegCurrent[i];

		if(keywordData[index].preprocess(0, x) == false){
			logging.warn("skipping bad keyword prediction model");
			continue;
		}

		model.input() = x;

		if(model.calculate(false) == false){
			logging.warn("skipping bad keyword prediction model");
			continue;
		}

		x = model.output();

		if(keywordData[index].invpreprocess(1, x) == false){
			logging.warn("skipping bad keyword prediction model");
			continue;
		}

		// now we have prediction x to the response to the given keyword
		// calculates error (weighted distance to the target state)

		auto delta = target - x;

		for(unsigned int i=0;i<delta.size();i++)
			delta[i] /= targetVariance[i];

		auto error = delta.norm();

		if(error < bestError){
			math::convert(bestError, error);
			bestKeyword = index;
		}

		engine_pollEvents(); // polls for incoming events in case there are lots of models
	}


	bestError = std::numeric_limits<double>::infinity();

	for(unsigned int index=0;index<pictureModels.size();index++){
		whiteice::nnetwork<>& model = pictureModels[index];

		if(model.input_size() != eegCurrent.size() || model.output_size() != eegTarget.size()){
			logging.warn("skipping bad picture prediction model");
			continue; // bad model/data => ignore
		}

		math::vertex<> x(eegCurrent.size());
		for(unsigned int i=0;i<x.size();i++)
			x[i] = eegCurrent[i];

		if(pictureData[index].preprocess(0, x) == false){
			logging.warn("skipping bad picture prediction model");
			continue;
		}

		model.input() = x;

		if(model.calculate(false) == false){
			logging.warn("skipping bad picture prediction model");
			continue;
		}

		x = model.output();

		if(pictureData[index].invpreprocess(1, x) == false){
			logging.warn("skipping bad picture prediction model");
			continue;
		}

		// now we have prediction x to the response to the given keyword
		// calculates error (weighted distance to the target state)

		auto delta = target - x;

		for(unsigned int i=0;i<delta.size();i++)
			delta[i] /= targetVariance[i];

		auto error = delta.norm();

		if(error < bestError){
			math::convert(bestError, error);
			bestPicture = index;
		}

		engine_pollEvents(); // polls for incoming events in case there are lots of models
	}

	if(bestKeyword < 0 || bestPicture <=0){
		logging.error("Execute command couldn't find picture or keyword command to show (no models?)");
		engine_pollEvents();
		return false;
	}
	else{
		char buffer[80];
		sprintf(buffer, "prediction model selected keyword/best picture: %s %s",
				keywords[bestKeyword].c_str(), pictures[bestPicture].c_str());
		logging.info(buffer);
	}

	// now we have best picture and keyword that is predicted
	// to change users state to target value: show them

	std::string message = keywords[bestKeyword];

	engine_showScreen(message, bestPicture);

	engine_updateScreen();
	engine_pollEvents();

	return true;
}


// executes program blindly based on Monte Carlo sampling and prediction models
// [only works for low dimensional target signals and well-trained models]
bool ResonanzEngine::engine_executeProgramMonteCarlo(const std::vector<float>& eegTarget,
		const std::vector<float>& eegTargetVariance)
{
	int bestKeyword = -1;
	int bestPicture = -1;
	float bestError = std::numeric_limits<double>::infinity();

	math::vertex<> target(eegTarget.size());
	math::vertex<> targetVariance(eegTargetVariance.size());

	for(unsigned int i=0;i<target.size();i++){
		target[i] = eegTarget[i];
		targetVariance[i] = eegTargetVariance[i];
	}

	if(mcsamples.size() <= 0)
		return false; // internal program error should have MC samples


	for(unsigned int index=0;index<keywordModels.size();index++){
		whiteice::nnetwork<>& model = keywordModels[index];

		if(model.input_size() != mcsamples[0].size() || model.output_size() != eegTarget.size()){
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

			if(model.calculate(x, x) == false){
				logging.warn("skipping bad keyword prediction model");
				continue;
			}

			if(keywordData[index].invpreprocess(1, x) == false){
				logging.warn("skipping bad keyword prediction model");
				continue;
			}

			// now we have prediction x to the response to the given keyword
			// calculates error (weighted distance to the target state)

			auto delta = target - x;

			for(unsigned int i=0;i<delta.size();i++)
				delta[i] /= targetVariance[i];

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
		whiteice::nnetwork<>& model = pictureModels[index];

		if(model.input_size() != mcsamples[0].size() || model.output_size() != eegTarget.size()){
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

			if(model.calculate(x, x) == false){
				logging.warn("skipping bad picture prediction model");
				continue;
			}

			if(pictureData[index].invpreprocess(1, x) == false){
				logging.warn("skipping bad picture prediction model");
				continue;
			}

			// now we have prediction x to the response to the given keyword
			// calculates error (weighted distance to the target state)

			auto delta = target - x;

			for(unsigned int i=0;i<delta.size();i++)
				delta[i] /= targetVariance[i];

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

	if(bestKeyword < 0 || bestPicture <=0){
		logging.error("Execute command couldn't find picture or keyword command to show (no models?)");
		engine_pollEvents();
		return false;
	}
	else{
		char buffer[80];
		sprintf(buffer, "prediction model selected keyword/best picture: %s %s",
				keywords[bestKeyword].c_str(), pictures[bestPicture].c_str());
		logging.info(buffer);
	}


	// calculates estimates for MCMC samples [user state after stimulus] for the next round
	// we have two separate prediction models for this: keywords and pictures:
	// * for each sample we decide randomly which prediction model to use.
	// * additionally, we now postprocess samples to stay within [0,1] interval
	//   as our eeg-metasignals are always within [0,1]-range
	{
		for(auto& x : mcsamples){

			if((rand()&1) == 0){ // use keyword model to predict the outcome
				const auto& index = bestKeyword;

				whiteice::nnetwork<>& model = keywordModels[index];

				if(keywordData[index].preprocess(0, x) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}

				model.input() = x;

				if(model.calculate(false) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}

				x = model.output();

				if(keywordData[index].invpreprocess(1, x) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}
			}
			else{ // use picture model to predict the outcome
				const auto& index = bestPicture;

				whiteice::nnetwork<>& model = pictureModels[index];

				if(pictureData[index].preprocess(0, x) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}

				model.input() = x;

				if(model.calculate(false) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}

				x = model.output();

				if(pictureData[index].invpreprocess(1, x) == false){
					logging.error("mc sampling: skipping bad keyword prediction model");
					continue;
				}
			}

			// post-process predicted x to be within [0,1] interval
			for(unsigned int i=0;i<x.size();i++){
				if(x[i] < 0.0f) x[i] = 0.0f;
				else if(x[i] > 1.0f) x[i] = 1.0f;
			}

			engine_pollEvents(); // polls for incoming events in case there are lots of samples
		}
	}

	// now we have best picture and keyword that is predicted
	// to change users state to target value: show them


	std::string message = keywords[bestKeyword];

	engine_showScreen(message, bestPicture);

	engine_updateScreen();
	engine_pollEvents();

	return true;
}


bool ResonanzEngine::engine_optimizeModels(unsigned int& currentPictureModel, unsigned int& currentKeywordModel)
{
	if(currentPictureModel < pictureData.size()){

		if(optimizer == nullptr){ // first model to be optimized (no need to store the previous result)
			whiteice::math::vertex<> w;

			nn->randomize();
			nn->exportdata(w);

			optimizer = new whiteice::LBFGS_nnetwork<>(*nn, pictureData[currentPictureModel], false, false);

			{
				char buffer[1024];
				sprintf(buffer, "resonanz model optimization started: picture %d database size: %d",
						currentPictureModel, pictureData[currentPictureModel].size(0));
				logging.info(buffer);
			}

			optimizer->minimize(w);
		}
		else{
			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(optimizer->isRunning() == false || optimizer->solutionConverged() == true || iterations >= 1000){
				// gets finished solution

				optimizer->stopComputation();
				optimizer->getSolution(w, error, iterations);

				{
					char buffer[1024];
					sprintf(buffer, "resonanz model optimization stopped. picture: %d iterations: %d error: %f",
							currentPictureModel, iterations, error.c[0]);
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
						calculateHashName(pictures[currentPictureModel] + eeg->getDataSourceName()) + ".model";
				nn->importdata(w);
				if(nn->save(dbFilename) == false)
					logging.error("saving nn configuration file failed");

				delete optimizer;
				optimizer = nullptr;

				// starts new computation
				currentPictureModel++;
				if(currentPictureModel < pictures.size()){
					nn->randomize();
					nn->exportdata(w);

					{
						char buffer[1024];
						sprintf(buffer, "resonanz model optimization started: picture %d database size: %d",
								currentPictureModel, pictureData[currentPictureModel].size(0));
						logging.info(buffer);
					}

					optimizer = new whiteice::LBFGS_nnetwork<>(*nn, pictureData[currentPictureModel], false, false);
					optimizer->minimize(w);
				}
			}
		}
	}
	else if(currentKeywordModel < keywords.size()){

		if(optimizer == nullptr){ // first model to be optimized (no need to store the previous result)
			whiteice::math::vertex<> w;

			nn->randomize();
			nn->exportdata(w);

			optimizer = new whiteice::LBFGS_nnetwork<>(*nn, keywordData[currentKeywordModel], false, false);
			optimizer->minimize(w);

			{
				char buffer[1024];
				sprintf(buffer, "resonanz model optimization started: keyword %d database size: %d",
						currentKeywordModel, keywordData[currentKeywordModel].size(0));
				logging.info(buffer);
			}

		}
		else{
			whiteice::math::blas_real<float> error = 1000.0f;
			whiteice::math::vertex<> w;
			unsigned int iterations = 0;

			optimizer->getSolution(w, error, iterations);

			if(optimizer->isRunning() == false || optimizer->solutionConverged() == true || iterations >= 1000){
				// gets finished solution

				optimizer->stopComputation();

				optimizer->getSolution(w, error, iterations);


				{
					char buffer[1024];
					sprintf(buffer, "resonanz model optimization stopped. keyword: %d iterations: %d error: %f",
							currentKeywordModel, iterations, error.c[0]);
					logging.info(buffer);
				}

				// saves optimization results to a file
				std::string dbFilename = currentCommand.modelDir + "/" +
						calculateHashName(keywords[currentKeywordModel] + eeg->getDataSourceName()) + ".model";
				nn->importdata(w);
				if(nn->save(dbFilename) == false)
					logging.error("saving nn configuration file failed");

				delete optimizer;
				optimizer = nullptr;

				// starts new computation
				currentKeywordModel++;
				if(currentKeywordModel < keywords.size()){
					nn->randomize();
					nn->exportdata(w);

					{
						char buffer[1024];
						sprintf(buffer, "resonanz model optimization started: keyword %d database size: %d",
								currentKeywordModel, keywordData[currentKeywordModel].size(0));
						logging.info(buffer);
					}

					optimizer = new whiteice::LBFGS_nnetwork<>(*nn, keywordData[currentKeywordModel], false, false);
					optimizer->minimize(w);
				}
			}
		}
	}
	else{ // both picture and keyword models has been computed
		cmdStopCommand();
	}

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

	if(incomingCommand == nullptr) return false; // incomingCommand changed while acquiring mutex..

	currentCommand = *incomingCommand;

	delete incomingCommand;
	incomingCommand = nullptr;

	return true;
}


bool ResonanzEngine::engine_loadMedia(const std::string& picdir, const std::string& keyfile, bool loadData)
{
	std::vector<std::string> tempKeywords;

	// first we get picture names from directories and keywords from keyfile
	if(this->loadWords(keyfile, tempKeywords) == false)
		return false;

	std::vector<std::string> tempPictures;

	if(this->loadPictures(picdir, tempPictures) == false)
		return false;

	pictures = tempPictures;
	keywords = tempKeywords;

	for(unsigned int i=0;i<images.size();i++){
		if(images[i] != nullptr)
			SDL_FreeSurface(images[i]);
	}

	if(loadData){
		images.resize(pictures.size());

		for(unsigned int i=0;i<images.size();i++){
			images[i] = nullptr;
			engine_showScreen("Loading..", i);
			engine_pollEvents();
			engine_updateScreen();
		}
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

	// loads databases into memory or initializes new ones
	for(unsigned int i=0;i<keywords.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";

		if(keywordData[i].load(dbFilename) == false){
			keywordData[i].createCluster(name1, eeg->getNumberOfSignals());
			keywordData[i].createCluster(name2, eeg->getNumberOfSignals());
		}
	}


	for(unsigned int i=0;i<pictures.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(pictures[i] + eeg->getDataSourceName()) + ".ds";

		if(pictureData[i].load(dbFilename) == false){
			pictureData[i].createCluster(name1, eeg->getNumberOfSignals());
			pictureData[i].createCluster(name2, eeg->getNumberOfSignals());
		}
	}

	return true;
}


bool ResonanzEngine::engine_storeMeasurement(unsigned int pic, unsigned int key, const std::vector<float>& eegBefore, const std::vector<float>& eegAfter)
{
	std::vector< whiteice::math::blas_real<float> > t1, t2;
	t1.resize(eegBefore.size());
	t2.resize(eegAfter.size());

	for(unsigned int i=0;i<t1.size();i++){
		t1[i] = eegBefore[i];
		t2[i] = eegAfter[i];
	}

	keywordData[key].add(0, t1);
	keywordData[key].add(1, t2);

	pictureData[pic].add(0, t1);
	pictureData[pic].add(1, t2);

	return true;
}


bool ResonanzEngine::engine_saveDatabase(const std::string& modelDir)
{
	std::lock_guard<std::mutex> lock(database_mutex);

	// saves databases from memory
	for(unsigned int i=0;i<keywordData.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";

		if(keywordData[i].save(dbFilename) == false)
			return false;
	}


	for(unsigned int i=0;i<pictureData.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(pictures[i] + eeg->getDataSourceName()) + ".ds";

		if(pictureData[i].save(dbFilename) == false)
			return false;
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
				sprintf(buffer, "%.2x", hash160[i]);
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


bool ResonanzEngine::engine_showScreen(const std::string& message, unsigned int picture)
{
	SDL_Surface* surface = SDL_GetWindowSurface(window);
	if(surface == nullptr)
		return false;

	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));

	int bgcolor = 0;

	if(picture < pictures.size()){ // shows a picture
		if(images[picture] == NULL){
			SDL_Surface* image = IMG_Load(pictures[picture].c_str());
			SDL_Rect imageRect;
			SDL_Surface* scaled = 0;

			if(image != 0){
				if((image->w) > (image->h)){
					double wscale = ((double)SCREEN_WIDTH)/((double)image->w);
					// scaled = zoomSurface(image, wscale, wscale, SMOOTHING_ON);

					scaled = SDL_CreateRGBSurface(0, (int)(image->w*wscale), (int)(image->h*wscale), 32,
							0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

					SDL_BlitScaled(image, NULL, scaled, NULL);

				}
				else{
					double hscale = ((double)SCREEN_HEIGHT)/((double)image->h);
					// scaled = zoomSurface(image, hscale, hscale, SMOOTHING_ON);

					scaled = SDL_CreateRGBSurface(0, (int)(image->w*hscale), (int)(image->h*hscale), 32,
							0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

					SDL_BlitScaled(image, NULL, scaled, NULL);
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

				SDL_BlitSurface(images[picture], NULL, surface, &imageRect);
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

			SDL_BlitSurface(images[picture], NULL, surface, &imageRect);
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

		SDL_Surface* msg = TTF_RenderUTF8_Blended(font, message.c_str(), color);

		SDL_Rect messageRect;

		messageRect.x = (SCREEN_WIDTH - msg->w)/2;
		messageRect.y = (SCREEN_HEIGHT - msg->h)/2;
		messageRect.w = msg->w;
		messageRect.h = msg->h;


		SDL_BlitSurface(msg, NULL, surface, &messageRect);

		if(video && programStarted > 0){
			auto t1 = std::chrono::system_clock::now().time_since_epoch();
			auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();

			video->insertFrame((t1ms - programStarted), surface);
		}


		SDL_FreeSurface(msg);
	}

	return true;
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
	if(window != nullptr)
		SDL_UpdateWindowSurface(window);
}


// initializes SDL libraries to be used (graphics, font, music)
bool ResonanzEngine::engine_SDL_init(const std::string& fontname)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_DisplayMode mode;

	if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
		SCREEN_WIDTH = mode.w;
		SCREEN_HEIGHT = mode.h;
	}
	else{
		return false; // something went wrong
	}

	if(TTF_Init() != 0){
		char buffer[80];
		sprintf(buffer, "TTF_Init failed: %s\n", TTF_GetError());
		logging.error(buffer);
		throw std::runtime_error("TTF_Init() failed.");
	}

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;

	if(IMG_Init(flags) != flags){
		char buffer[80];
		sprintf(buffer, "IMG_Init failed: %s\n", IMG_GetError());
		logging.error(buffer);
		IMG_Quit();
		throw std::runtime_error("IMG_Init() failed.");
	}

	flags = MIX_INIT_OGG | MIX_INIT_MP3;

	audioEnabled = true;

	if(Mix_Init(flags) != flags){
		char buffer[80];
		sprintf(buffer, "Mix_Init failed: %s\n", Mix_GetError());
		logging.warn(buffer);
		audioEnabled = false;
		/*
		IMG_Quit();
		Mix_Quit();
		throw std::runtime_error("Mix_Init() failed.");
		*/
	}

	double fontSize = 100.0*sqrt(((float)(SCREEN_WIDTH*SCREEN_HEIGHT))/(640.0*480.0));
	unsigned int fs = (unsigned int)fontSize;
	if(fs <= 0) fs = 10;

	font = 0;
	font = TTF_OpenFont(fontname.c_str(), fs);

	if(font == 0){
		char buffer[80];
		sprintf(buffer, "TTF_OpenFont failure (%s): %s\n", fontname.c_str() , TTF_GetError());
		logging.error(buffer);
		IMG_Quit();
		Mix_Quit();
		throw std::runtime_error("TTF_OpenFont() failed.");
	}

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1){
		audioEnabled = false;
		char buffer[80];
		sprintf(buffer, "ERROR: Cannot open SDL mixer: %s.\n", Mix_GetError());
		logging.warn(buffer);
	}

	return true;
}


bool ResonanzEngine::engine_SDL_deinit()
{
	if(audioEnabled)
		SDL_CloseAudio();

	TTF_CloseFont(font);
	IMG_Quit();

	if(audioEnabled)
		Mix_Quit();

	TTF_Quit();
	SDL_Quit();

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
			sprintf(buffer, "sdl-music: loading audio file failed: %s", audioFile.c_str());
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
			if(buffer[i] == '\n')
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


std::string ResonanzEngine::analyzeModel(const std::string& modelDir)
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

	std::lock_guard<std::mutex> lock(database_mutex);

	for(auto filename : databaseFiles){
		// calculate statistics
		whiteice::dataset<> ds;
		std::string fullname = modelDir + "/" + filename;
		if(ds.load(fullname) == false){
			failed++;
			continue; // couldn't load this dataset
		}

		if(ds.size() < minDSSamples) minDSSamples = ds.size();
		avgDSSamples += ds.size();
		N++;

		std::string modelFilename = fullname.substr(0, fullname.length()-3) + ".model";

		// check if there is a model file and load it into memory and calculate average error
		whiteice::nnetwork<> nnet;
		if(nnet.load(modelFilename))
			models++;

	}


	if(N > 0){
		avgDSSamples /= N;
		double modelPercentage = 100*models/((double)N);

		char buffer[1024];
		sprintf(buffer, "%d entries (%.0f%% has a model). samples(avg): %.2f, samples(min): %d",
				N, modelPercentage, avgDSSamples, minDSSamples);

		return buffer;
	}
	else{
		char buffer[1024];
		sprintf(buffer, "%d entries (0%% has a model). samples(avg): %.2f, samples(min): %d",
				0, 0.0, 0);

		return buffer;
	}
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
	float num_keywords = 0.0f;
	float num_pictures = 0.0f;

	// 2. loads dataset files (.ds) one by one if possible and calculates mean delta
	whiteice::dataset<> data;

	// loads databases into memory or initializes new ones
	for(unsigned int i=0;i<keywords.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(keywords[i] + eeg->getDataSourceName()) + ".ds";

		data.clearAll();

		if(data.load(dbFilename) == true){
			if(data.getNumberOfClusters() == 2){
				float delta = 0.0f;

				for(unsigned int j=0;j<data.size(0);j++){
					auto d = data.access(1, j) - data.access(0, j);
					delta += d.norm().c[0] / data.size(0);
				}

				std::pair<float, std::string> p;
				p.first  = -delta;
				p.second = keywords[i];
				keywordDeltas.insert(p);

				mean_delta_keywords += delta;
				var_delta_keywords  += delta*delta;
				num_keywords++;
			}
		}
	}

	mean_delta_keywords /= num_keywords;
	var_delta_keywords  /= num_keywords;
	var_delta_keywords  -= mean_delta_keywords*mean_delta_keywords;
	var_delta_keywords  *= num_keywords/(num_keywords - 1.0f);

	for(unsigned int i=0;i<pictureFiles.size();i++){
		std::string dbFilename = modelDir + "/" + calculateHashName(pictureFiles[i] + eeg->getDataSourceName()) + ".ds";

		data.clearAll();

		if(data.load(dbFilename) == true){
			if(data.getNumberOfClusters() == 2){
				float delta = 0.0f;

				for(unsigned int j=0;j<data.size(0);j++){
					auto d = data.access(1, j) - data.access(0, j);
					delta += d.norm().c[0] / data.size(0);
				}

				std::pair<float, std::string> p;
				p.first  = -delta;
				p.second = pictureFiles[i];
				pictureDeltas.insert(p);

				mean_delta_pictures += delta;
				var_delta_pictures  += delta*delta;
				num_pictures++;
			}
		}
	}

	mean_delta_pictures /= num_pictures;
	var_delta_pictures  /= num_pictures;
	var_delta_pictures  -= mean_delta_pictures*mean_delta_pictures;
	var_delta_pictures  *= num_pictures/(num_pictures - 1.0f);

	// 3. sorts deltas/keyword delta/picture (use <multimap> for automatic ordering) and prints the results

	std::string report = "";
	const unsigned int BUFSIZE = 512;
	char buffer[BUFSIZE];

	snprintf(buffer, BUFSIZE, "Picture delta: %.2f stdev(delta): %.2f\n", mean_delta_pictures, sqrt(var_delta_pictures));
	report += buffer;
	snprintf(buffer, BUFSIZE, "Keyword delta: %.2f stdev(delta): %.2f\n", mean_delta_keywords, sqrt(var_delta_keywords));
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
		snprintf(buffer, 80, "Program performance (average error): %.2f.\n", rms);
		std::string result = buffer;

		return result;
	}
	else{
		return "No program performance data available.\n";
	}
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
