/*
 * ResonanzEngine.cpp
 *
 *  Created on: 13.6.2015
 *      Author: Tomas
 */

#include "ResonanzEngine.h"
#include <chrono>
#include <exception>
#include <math.h>

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
	workerThread = new std::thread(std::bind(ResonanzEngine::engine_loop, this));
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
void ResonanzEngine::engine_loop()
{
	// TODO autodetect good values based on windows screen resolution
	SCREEN_WIDTH  = 800;
	SCREEN_HEIGHT = 600;
	const std::string fontname = "Vera.ttf";


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

			engine_sleep(1000, "resonanz-engine: re-trying to initialize graphics..");
		}

		if(thread_is_running == false){
			if(initialized) engine_SDL_deinit();
			return;
		}
	}


	while(thread_is_running){
		ResonanzCommand prevCommand = currentCommand;
		if(engine_checkIncomingCommand() == true){
			printf("RESONANZ-ENGINE: NEW COMMAND\n");
			fflush(stdout);
			// we must make engine state transitions, state transfer from the previous command to the new command

			// first checks if we want to have open graphics window and opens one if needed
			if(currentCommand.showScreen == true && prevCommand.showScreen == false){
				if(window != nullptr) SDL_DestroyWindow(window);

				window = SDL_CreateWindow(windowTitle.c_str(),
							    SDL_WINDOWPOS_CENTERED,
							    SDL_WINDOWPOS_CENTERED,
							    SCREEN_WIDTH, SCREEN_HEIGHT,
							    SDL_WINDOW_SHOWN);

				if(window != nullptr){
					SDL_Surface* icon = IMG_Load(iconFile.c_str());
					if(icon != nullptr){
						SDL_SetWindowIcon(window, icon);
						SDL_FreeSurface(icon);
					}

					SDL_Surface* surface = SDL_GetWindowSurface(window);
					SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
					SDL_UpdateWindowSurface(window);
				}

				SDL_RaiseWindow(window);
			}
			else if(currentCommand.showScreen == true && prevCommand.showScreen == true){
				// just empties current window with blank (black) screen
				if(window == nullptr){
					window = SDL_CreateWindow(windowTitle.c_str(),
								    SDL_WINDOWPOS_CENTERED,
								    SDL_WINDOWPOS_CENTERED,
								    SCREEN_WIDTH, SCREEN_HEIGHT,
								    SDL_WINDOW_SHOWN);
				}

				if(window != nullptr){
					SDL_Surface* icon = IMG_Load(iconFile.c_str());
					if(icon != nullptr){
						SDL_SetWindowIcon(window, icon);
						SDL_FreeSurface(icon);
					}

					SDL_Surface* surface = SDL_GetWindowSurface(window);
					SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
					SDL_UpdateWindowSurface(window);
				}

				SDL_RaiseWindow(window);
			}
			else if(currentCommand.showScreen == false){
				if(window != nullptr) SDL_DestroyWindow(window);
				window = nullptr;
			}

			// (re)loads media resources (pictures, keywords) if we want to do stimulation
			if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM || currentCommand.command == ResonanzCommand::CMD_DO_MEASURE){
				engine_loadMedia(currentCommand.pictureDir, currentCommand.keywordsFile);
			}

		}

		// executes current command
		if(currentCommand.command == ResonanzCommand::CMD_DO_NOTHING){
			engine_sleep(100, "resonanz-engine: sleeping.."); // 100ms
		}
		else if(currentCommand.command == ResonanzCommand::CMD_DO_RANDOM){
			// FIXME current just shows keywords without pictures

			if(keywords.size() > 0){
				unsigned int key = rand() % keywords.size();
				unsigned int pic = rand() % pictures.size();

				if(engine_showScreen(keywords[key], pic) == false){

				}
			}
		}


		engine_pollEvents(); // polls for events
		engine_updateScreen(); // always updates window if it exists
	}

	if(window != nullptr)
		SDL_DestroyWindow(window);

	engine_SDL_deinit();

}


void ResonanzEngine::engine_sleep(int msecs, const std::string& state)
{
	// sleeps for given number of milliseconds, updates engineState
	status_mutex.lock();
	engineState = state;
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


bool ResonanzEngine::engine_loadMedia(const std::string& picdir, const std::string& keyfile)
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

	images.resize(pictures.size());

	for(unsigned int i=0;i<images.size();i++){
	    images[i] = nullptr;
	    engine_showScreen("Loading..", i);
	    engine_pollEvents();
	    engine_updateScreen();
	}

	return true;
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
		SDL_Color red   = { 255, 0  , 0   };
		SDL_Color green = { 0  , 255, 0   };
		SDL_Color blue  = { 0  , 0  , 255 };
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

	if(TTF_Init() != 0){
		printf("TTF_Init failed: %s\n", TTF_GetError());
		fflush(stdout);
		throw std::runtime_error("TTF_Init() failed.");
	}

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;

	if(IMG_Init(flags) != flags){
		printf("IMG_Init failed: %s\n", IMG_GetError());
		fflush(stdout);
		IMG_Quit();
		throw std::runtime_error("IMG_Init() failed.");
	}

	flags = MIX_INIT_OGG;

	if(Mix_Init(flags) != flags){
		printf("Mix_Init failed: %s\n", Mix_GetError());
		fflush(stdout);
		IMG_Quit();
		Mix_Quit();
		throw std::runtime_error("Mix_Init() failed.");
	}

	double fontSize = 100.0*sqrt(((float)(SCREEN_WIDTH*SCREEN_HEIGHT))/(640.0*480.0));
	unsigned int fs = (unsigned int)fontSize;
	if(fs <= 0) fs = 10;

	font = 0;
	font = TTF_OpenFont(fontname.c_str(), fs);

	if(font == 0){
		printf("TTF_OpenFont failure (%s): %s\n", fontname.c_str() , TTF_GetError());
		fflush(stdout);
		IMG_Quit();
		Mix_Quit();
		throw std::runtime_error("TTF_OpenFont() failed.");
	}

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1){
		IMG_Quit();
		Mix_Quit();
		printf("ERROR: Cannot open SDL mixer: %s.\n", Mix_GetError());

		throw std::runtime_error("Mix_OpenAudio() failed.");
	}

	return true;
}


bool ResonanzEngine::engine_SDL_deinit()
{
  SDL_CloseAudio();
  TTF_CloseFont(font);
  IMG_Quit();
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();

  return true;
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




bool ResonanzEngine::loadWords(const std::string filename, std::vector<std::string>& words)
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


bool ResonanzEngine::loadPictures(const std::string directory, std::vector<std::string>& pictures)
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
