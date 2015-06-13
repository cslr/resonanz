/*
 * ResonanzEngine.cpp
 *
 *  Created on: 13.6.2015
 *      Author: Tomas
 */

#include "ResonanzEngine.h"
#include <chrono>
#include <exception>


namespace whiteice {
namespace resonanz {

ResonanzEngine::ResonanzEngine()
{
	std::lock_guard<std::mutex> lock(thread_mutex);

	{
		std::lock_guard<std::mutex> lock(status_mutex);
		engineState = "resonanz-engine: starting...";
	}

	workerThread = nullptr;
	thread_is_running = true;

	{
		std::lock_guard<std::mutex> lock(command_mutex);
		incomingCommand = nullptr;
		currentCommand.command = ResonanzCommand::CMD_DO_NOTHING;
		currentCommand.showScreen = false;
	}



	// starts updater thread thread
	workerThread = new std::thread(std::bind(ResonanzEngine::updateLoop, this));
	workerThread->detach();
}

ResonanzEngine::~ResonanzEngine()
{
	std::lock_guard<std::mutex> lock(thread_mutex);

	{
		std::lock_guard<std::mutex> lock(status_mutex);
		engineState = "resonanz-engine: stopping...";
	}

	thread_is_running = false;
	if(workerThread == nullptr)
		return; // no thread is running

	// waits for thread to stop
	std::chrono::milliseconds duration(1000); // 1000ms (thread sleep/working period is something like < 100ms)
	std::this_thread::sleep_for(duration);

	// deletes thread whether it is still running or not
	delete workerThread;
	workerThread = nullptr;

	{
		std::lock_guard<std::mutex> lock(status_mutex);
		engineState = "resonanz-engine: not running";
	}
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

		{
			std::lock_guard<std::mutex> lock(status_mutex);
			engineState = "resonanz-engine: restarting...";
		}

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
		workerThread = new std::thread(std::bind(ResonanzEngine::updateLoop, this));
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


bool ResonanzEngine::cmdOptimizeModel(const std::string& modelDir) throw()
{
	if(modelDir.length() <= 0)
		return false;

	// TODO check that those directories and files actually exist

	std::lock_guard<std::mutex> lock(command_mutex);
	if(incomingCommand != nullptr) delete incomingCommand;
	incomingCommand = new ResonanzCommand();

	incomingCommand->command = ResonanzCommand::CMD_DO_OPTIMIZE;
	incomingCommand->showScreen = false;
	incomingCommand->pictureDir = "";
	incomingCommand->keywordsFile = "";
	incomingCommand->modelDir = modelDir;

	return true;
}


bool ResonanzEngine::cmdStopOptimizeModel() throw()
{
	// just sets CMD_DO_NOTHING state
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


// main worker thread loop to execute commands
void ResonanzEngine::updateLoop()
{
	while(thread_is_running){
		ResonanzCommand prevCommand = currentCommand;
		if(engine_checkIncomingCommand() == true){
			printf("RESONANZ-ENGINE: NEW COMMAND\n");
			fflush(stdout);
			// we must make engine state transitions, state transfer from the previous command to the new command
		}

		// executes current command
		if(currentCommand.command == ResonanzCommand::CMD_DO_NOTHING){
			engine_sleep(100); // 100ms
		}

	}
}


void ResonanzEngine::engine_sleep(int msecs)
{
	// sleeps for given number of milliseconds, updates engineState
	status_mutex.lock();
	engineState = "resonanz-engine: sleeping";
	status_mutex.unlock();

	// currently just sleeps()
	std::chrono::milliseconds duration(msecs);
	std::this_thread::sleep_for(duration);
}


bool ResonanzEngine::engine_checkIncomingCommand()
{
	if(incomingCommand == nullptr) return false;

	std::lock_guard<std::mutex> lock(command_mutex);

	if(incomingCommand == nullptr) return false; // incomingCommand changed while acquiring mutex..

	currentCommand = *incomingCommand;

	delete incomingCommand;
	incomingCommand = nullptr;

	return true;
}




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
