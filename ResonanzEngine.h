/*
 * ResonanzEngine.h
 *
 *  Created on: 13.6.2015
 *      Author: Tomas
 */

#ifndef RESONANZENGINE_H_
#define RESONANZENGINE_H_

#include <string>
#include <thread>
#include <mutex>


namespace whiteice {
namespace resonanz {

/**
 * Resonanz command that is being executed or is given to the engine
 */
class ResonanzCommand
{
public:
	ResonanzCommand();
	virtual ~ResonanzCommand();

	static const unsigned int CMD_DO_NOTHING  = 0;
	static const unsigned int CMD_DO_RANDOM   = 1;
	static const unsigned int CMD_DO_MEASURE  = 2;
	static const unsigned int CMD_DO_OPTIMIZE = 3;
	static const unsigned int CMD_DO_EXECUTE  = 4;

	unsigned int command;

	bool showScreen;
	std::string pictureDir;
	std::string keywordsFile;
	std::string modelDir;

	// TODO implement execute stimulus program parameters [float arrays for each signal + signal name and variance (target error) of the signal]
};


class ResonanzEngine {
public:
	ResonanzEngine();
	virtual ~ResonanzEngine();

	// what resonanz is doing right now [especially interesting if we are optimizing model]
	std::string getEngineStatus() throw();

	// resets resonanz-engine (worker thread stop and recreation)
	bool reset() throw();

	bool cmdDoNothing(bool showScreen);

	bool cmdRandom(const std::string& pictureDir, const std::string& keywordsFile) throw();

	bool cmdMeasure(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir) throw();

	bool cmdOptimizeModel(const std::string& modelDir) throw();

	bool cmdStopOptimizeModel() throw();

private:
	volatile bool thread_is_running;
	std::thread* workerThread;
	std::mutex   thread_mutex;

	ResonanzCommand currentCommand; // what the engine should be doing right now

	ResonanzCommand* incomingCommand;
	std::mutex command_mutex;

	std::string engineState;
	std::mutex status_mutex;

	// main worker thread loop to execute commands
	void updateLoop();

	// functions used by updateLoop():

	void engine_sleep(int msecs); // sleeps for given number of milliseconds, updates engineState
	bool engine_checkIncomingCommand();

};



} /* namespace resonanz */
} /* namespace whiteice */

#endif /* RESONANZENGINE_H_ */
