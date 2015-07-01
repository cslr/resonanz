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
#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <dinrhiw/dinrhiw.h>
#include "DataSource.h"
#include "SDLTheora.h"

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
	std::string audioFile;

	std::vector<std::string> signalName;
	std::vector< std::vector<float> > programValues;
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

	bool cmdOptimizeModel(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir) throw();

	bool cmdExecuteProgram(const std::string& pictureDir, const std::string& keywordsFile, const std::string& modelDir,
			const std::string& audioFile,
			const std::vector<std::string>& targetSignal, const std::vector< std::vector<float> >& program) throw();


	bool cmdStopCommand() throw();

	// returns true if resonaz-engine is executing some other command than do-nothing
	bool isBusy() throw();

	bool keypress(); // detects keypress from GUI

	// analyzes given measurements database and model performance
	std::string analyzeModel(const std::string& modelDir);

private:
	const std::string windowTitle = "Neuromancer NeuroStim";
	const std::string iconFile = "brain.png";

	volatile bool thread_is_running;
	std::thread* workerThread;
	std::mutex   thread_mutex;

	ResonanzCommand currentCommand; // what the engine should be doing right now

	ResonanzCommand* incomingCommand;
	std::mutex command_mutex;

	std::string engineState;
	std::mutex status_mutex;

	// main worker thread loop to execute commands
	void engine_loop();

	// functions used by updateLoop():

	void engine_setStatus(const std::string& msg) throw();

	void engine_sleep(int msecs); // sleeps for given number of milliseconds, updates engineState
	bool engine_checkIncomingCommand();

	bool engine_SDL_init(const std::string& fontname);
	bool engine_SDL_deinit();

	bool measureColor(SDL_Surface* image, SDL_Color& averageColor);

	bool engine_loadMedia(const std::string& picdir, const std::string& keyfile, bool loadData);
	bool engine_showScreen(const std::string& message, unsigned int picture);

	bool engine_playAudioFile(const std::string& audioFile);
	bool engine_stopAudioFile();

	void engine_pollEvents();

	void engine_updateScreen();

	SDL_Window* window = nullptr;
	int SCREEN_WIDTH, SCREEN_HEIGHT;
	TTF_Font* font = nullptr;
	bool audioEnabled = true; // false if opening audio failed
	Mix_Music* music = nullptr;

	bool keypressed = false;
	std::mutex keypress_mutex;

	static const unsigned int TICK_MS = 100; // how fast engine runs: engine measures ticks and executes (one) command only when tick changes
	static const unsigned int MEASUREMODE_DELAY_MS = 500; // how long each screen is shown when measuring response

	// media resources
	std::vector<std::string> keywords;
	std::vector<std::string> pictures;
	std::vector<SDL_Surface*> images;

	bool loadWords(const std::string filename, std::vector<std::string>& words);
	bool loadPictures(const std::string directory, std::vector<std::string>& pictures);


	bool engine_loadDatabase(const std::string& modelDir);
	bool engine_storeMeasurement(unsigned int pic, unsigned int key, const std::vector<float>& eegBefore, const std::vector<float>& eegAfter);
	bool engine_saveDatabase(const std::string& modelDir);
	std::string calculateHashName(std::string& filename);

	std::vector< whiteice::dataset<> > keywordData;
	std::vector< whiteice::dataset<> > pictureData;
	std::mutex database_mutex; // mutex to synchronize I/O access to dataset files

	DataSource* eeg = nullptr;


	bool engine_optimizeModels(unsigned int& currentPictureModel, unsigned int& currentKeywordModel);

	whiteice::LBFGS_nnetwork<>* optimizer = nullptr;
	whiteice::nnetwork<>* nn = nullptr;

	bool engine_loadModels(const std::string& modelDir); // loads prediction models for program execution, returns false in case of failure
	bool engine_executeProgram(const std::vector<float>& eegCurrent,
			const std::vector<float>& eegTarget, const std::vector<float>& eegTargetVariance);

	std::vector< whiteice::nnetwork<> > keywordModels;
	std::vector< whiteice::nnetwork<> > pictureModels;

	long long programStarted; // 0 = program has not been started
	SDLTheora* video; // used to encode program into video

};



} /* namespace resonanz */
} /* namespace whiteice */

#endif /* RESONANZENGINE_H_ */
