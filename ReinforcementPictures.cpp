

#include "ReinforcementPictures.h"

#include <exception>
#include <chrono>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include "Log.h"

namespace whiteice
{
  using namespace std::chrono;
  using namespace whiteice::resonanz;

  template <typename T>
  ReinforcementPictures<T>::ReinforcementPictures
  (const DataSource* dev,
   const whiteice::HMM& hmm,
   const whiteice::dataset<T>& timeseries,
   const std::vector<std::string>& pictures,
   const unsigned int DISPLAYTIME,
   const std::vector<double>& target,
   const std::vector<double>& targetVar) : RIFL_abstract<T>(pictures.size(),
							    dev->getNumberOfSignals())
  {
    if(dev == NULL || DISPLAYTIME == 0 || pictures.size() == 0){
      throw std::invalid_argument("ReinforcementPictures ctor: bad arguments");
    }
    
    keypresses = 0;
    display_thread = nullptr;

    this->dev = dev;
    this->hmm = hmm;
    this->timeseries = timeseries;
    this->pictures = pictures;
    this->DISPLAYTIME = DISPLAYTIME;
    this->target = target;
    this->targetVar = targetVar;

    this->fontname = "Vera.ttf";

    // starts display thread
    {
      running = true;
      display_thread = new thread(std::bind(&ReinforcementPictures<T>::display_thread,
					    this));
    }
  }

  template <typename T>
  ReinforcementPictures<T>::~ReinforcementPictures()
  {
    // stops display thread
    {
      std::lock_guard<std::mutex> lock(display_mutex);

      running = false;
      display_thread->join();
    }
  }

  
  template <typename T>
  bool ReinforcementPictures<T>::getKeypress()
  {
    return (keypresses > 0);
  }
  

  template <typename T>
  bool ReinforcementPictures<T>::getState(whiteice::math::vertex<T>& state)
  {
    state.resize(dev->getNumberOfSignals());

    std::vector<float> v;

    if(dev->data(v) == false) return false;
    
    for(unsigned int i=0;i<state.size();i++){
      state[i] = v[i];
    }

    return true;
  }

  template <typename T>
  bool ReinforcementPictures<T>::performAction(const unsigned int action,
					       whiteice::math::vertex<T>& newstate,
					       T& reinforcement)
  {
    // TODO IMPLEMENT ME (NOW THIS IS JUST DUMMY STUB)
    
    getState(newstate);
    
    reinforcement = rng.uniform();

    return true;
  }
  

  template <typename T>
  void ReinforcementPictures<T>::displayLoop()
  {
    // initialization
    logging.info("Starting SDL init (0)..");

    SDL_Init(0);

    logging.info("Starting SDL subsystem init (events, video, audio)..");

    if(SDL_InitSubSystem(SDL_INIT_EVENTS) != 0){
      logging.error("SDL_Init(EVENTS) failed.");
      SDL_Quit();
      running = false;
      return;
    }

    if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0){
      logging.error("SDL_Init(VIDEO) failed.");
      SDL_Quit();
      running = false;
      return;
    }

    if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0){
      logging.error("SDL_Init(AUDIO) failed.");
      SDL_Quit();
      running = false;
      return;
    }

    logging.info("SDL_Init() events, video, audio successful.");

    
    // opens SDL display

    SDL_Window* window = NULL;
    
    int W = 640;
    int H = 480;

    SDL_DisplayMode mode;

    if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
      W = (3*mode.w)/4;
      H = (3*mode.h)/4;
    }
    else{
      logging.error("SDL_GetCurrentDisplayMode() failed");
      running = false;
      SDL_Quit();
      return;
    }

    if(TTF_Init() != 0){
      char buffer[80];
      snprintf(buffer, 80, "TTF_Init failed: %s\n", TTF_GetError());
      logging.error(buffer);
      TTF_Quit();
      SDL_Quit();      
      running = false;      
      return;
    }
    else
      logging.info("Starting TTF_Init() done..");

    int flags = IMG_INIT_JPG | IMG_INIT_PNG;

    if(IMG_Init(flags) != flags){
      char buffer[80];
      snprintf(buffer, 80, "IMG_Init failed: %s\n", IMG_GetError());
      logging.error(buffer);
      IMG_Quit();
      TTF_Quit();
      SDL_Quit();
      running = false;
      return;
    }

    window = SDL_CreateWindow("Reinforcement Learning",
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      W, H,
			      SDL_WINDOW_ALWAYS_ON_TOP |
			      SDL_WINDOW_INPUT_FOCUS);

    if(window == NULL){
      logging.error("SDL_CreateWindow() failed\n");
      running = false;
      return;
    }

    SDL_RaiseWindow(window);
    SDL_UpdateWindowSurface(window);
    SDL_RaiseWindow(window);

    // loads font
    TTF_Font* font = NULL;

    {
      double fontSize = 100.0*sqrt(((float)(W*H))/(640.0*480.0));
      unsigned int fs = (unsigned int)fontSize;
      if(fs <= 0) fs = 10;
      
      font = TTF_OpenFont(fontname.c_str(), fs);
    }


    // loads all pictures
    std::vector<SDL_Surface*> images;
    images.resize(pictures.size());
    
    {
      for(unsigned int i=0;i<pictures.size();i++)
	images[i] = NULL;
      
      for(unsigned int i=0;i<pictures.size() && running;i++)
      {
	SDL_Surface* image = IMG_Load(pictures[i].c_str());
	

	if(image == NULL){
	  char buffer[120];
	  snprintf(buffer, 120, "Loading image FAILED (%s): %s",
		   SDL_GetError(), pictures[i].c_str());
	  logging.warn(buffer);
	  printf("ERROR: %s\n", buffer);

	  image = SDL_CreateRGBSurface(0, W, H, 32,
				       0x00FF0000, 0x0000FF00, 0x000000FF,
				       0xFF000000);

	  if(image == NULL){
	    logging.error("Creating RGB surface failed");
	    IMG_Quit();
	    TTF_Quit();
	    SDL_Quit();

	    running = false;
	    return;
	  }
	  
	  SDL_FillRect(image, NULL, SDL_MapRGB(image->format, 0, 0, 0));
	}

	SDL_Rect imageRect;
	SDL_Surface* scaled = NULL;

	if(image->w >= image->h){
	  double wscale = ((double)W)/((double)image->w);
	  
	  scaled = SDL_CreateRGBSurface(0,
					(int)(image->w*wscale),
					(int)(image->h*wscale), 32,
					0x00FF0000, 0x0000FF00, 0x000000FF,
					0xFF000000);

	  if(scaled == NULL){
	    logging.error("Creating RGB surface failed");
	    IMG_Quit();
	    TTF_Quit();
	    SDL_Quit();

	    running = false;
	    return;
	  }

	  if(SDL_BlitScaled(image, NULL, scaled, NULL) != 0)
	    logging.warn("SDL_BlitScaled fails");
	}
	else{
	  double hscale = ((double)H)/((double)image->h);
	  
	  scaled = SDL_CreateRGBSurface(0,
					(int)(image->w*hscale),
					(int)(image->h*hscale), 32,
					0x00FF0000, 0x0000FF00, 0x000000FF,
					0xFF000000);

	  if(scaled == NULL){
	    logging.error("Creating RGB surface failed");
	    IMG_Quit();
	    TTF_Quit();
	    SDL_Quit();

	    running = false;
	    return;
	  }
	  
	  if(SDL_BlitScaled(image, NULL, scaled, NULL) != 0)
	    logging.warn("SDL_BlitScaled fails");
	}

	images[i] = scaled;

	if(image) SDL_FreeSurface(image);

	// displays pictures that are being loaded
	{
	  imageRect.w = scaled->w;
	  imageRect.h = scaled->h;
	  imageRect.x = (W - scaled->w)/2;
	  imageRect.y = (H - scaled->h)/2;

	  SDL_Surface* surface = SDL_GetWindowSurface(window);

	  if(surface){
	    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
	    SDL_BlitSurface(scaled, NULL, surface, &imageRect);

	    if(font){
	      SDL_Color white = { 255, 255, 255 };
	      
	      char message[80];
	      snprintf(message, 80, "%d/%d", i+1, pictures.size());
	      
	      SDL_Surface* msg = TTF_RenderUTF8_Blended(font, message, white);
	      
	      if(msg != NULL){
		SDL_Rect messageRect;
		
		messageRect.x = (W - msg->w)/2;
		messageRect.y = (H - msg->h)/2;
		messageRect.w = msg->w;
		messageRect.h = msg->h;
		
		SDL_BlitSurface(msg, NULL, surface, &messageRect);
		
		SDL_FreeSurface(msg);
	      }
	    }

	    SDL_UpdateWindowSurface(window);
	    SDL_ShowWindow(window);
	    SDL_FreeSurface(surface);
	  }
	}

	SDL_Event event;

	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    keypresses++;
	    continue;
	  }
	}
      }

    }
    
    
    
    while(running){
      SDL_Event event;

      while(SDL_PollEvent(&event)){
	if(event.type == SDL_KEYDOWN){
	  keypresses++;
	  continue;
	}
      }
      
      // currently just shows pictures randomly
      const unsigned int index = rng.rand() % images.size();
      
      {
	SDL_Surface* surface = SDL_GetWindowSurface(window);

	if(surface != NULL){
	  SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
	  
	  SDL_Rect imageRect;
	  imageRect.w = images[index]->w;
	  imageRect.h = images[index]->h;
	  imageRect.x = (W - images[index]->w)/2;
	  imageRect.y = (H - images[index]->h)/2;
	  
	  SDL_BlitSurface(images[index], NULL, surface, &imageRect);

	  SDL_UpdateWindowSurface(window);
	  SDL_ShowWindow(window);
	  SDL_FreeSurface(surface);
	}
	
      }
      
      
    }

    
    SDL_DestroyWindow(window);

    if(font){
      TTF_CloseFont(font);
      font = NULL;
    }

    logging.info("SDL deinitialization started");
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    logging.info("SDL deinitialization.. DONE");
    
    
  }
  
  
  
  template class ReinforcementPictures< math::blas_real<float> >;
  template class ReinforcementPictures< math::blas_real<double> >;
};
