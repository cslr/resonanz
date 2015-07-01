
#include "ResonanzShow.h"

#include <exception>
#include <stdexcept>
#include <math.h>

#define SMOOTHING_ON  1
#define SMOOTHING_OFF 0

SDL_Surface* zoomSurface(SDL_Surface* image, double xscale, double yscale, int flag);


ResonanzShow::ResonanzShow(int WIDTH, int HEIGHT)
{
  SDL_Init(SDL_INIT_EVERYTHING);
  
  if(TTF_Init() != 0){
    printf("TTF_Init failed: %s\n", TTF_GetError());
    throw std::runtime_error("TTF_Init() failed.");
  }
  
  int flags = IMG_INIT_JPG | IMG_INIT_PNG;
  
  if(IMG_Init(flags) != flags){
    printf("IMG_Init failed: %s\n", IMG_GetError());
    IMG_Quit();
    throw std::runtime_error("IMG_Init() failed.");
  }
  
  flags = MIX_INIT_OGG;
  
  if(Mix_Init(flags) != flags){
    printf("Mix_Init failed: %s\n", Mix_GetError());
    IMG_Quit();
    Mix_Quit();
    throw std::runtime_error("Mix_Init() failed.");
  }
  
  double fontSize = 100.0*sqrt(((float)(WIDTH*HEIGHT))/(640.0*480.0));
  unsigned int fs = (unsigned int)fontSize;
  if(fs <= 0) fs = 10;
  
  font = 0;
  font = TTF_OpenFont("Vera.ttf", fs);
  if(font == 0){
    printf("TTF_OpenFont failure: %s\n", TTF_GetError());
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
  
  
  
  window = 0;
  this->WIDTH  = WIDTH;
  this->HEIGHT = HEIGHT;
  
  window = SDL_CreateWindow("Resonanz", 
			    SDL_WINDOWPOS_CENTERED,
			    SDL_WINDOWPOS_CENTERED,
			    WIDTH, HEIGHT,
			    SDL_WINDOW_SHOWN);

  if(window == 0){
    printf("Could not create window.\n");
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    IMG_Quit();
    Mix_Quit();
    SDL_CloseAudio();
    throw std::runtime_error("SDL: Could not create window.");
  }
  
  // black window initially
  SDL_Surface* surface = SDL_GetWindowSurface(window);
  SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
  SDL_UpdateWindowSurface(window);
  
  images.resize(pictures.size());
  
  for(unsigned int i=0;i<images.size();i++)
    images[i] = NULL;
  
  
  tracks.resize(sounds.size());
  
  for(unsigned int i=0;i<tracks.size();i++)
    tracks[i] = NULL;
  
}


ResonanzShow::~ResonanzShow()
{
  for(unsigned int i=0;i<images.size();i++){
    if(images[i] != NULL){
      SDL_FreeSurface(images[i]);
      images[i] = NULL;
    }
  }

  
  for(unsigned int i=0;i<tracks.size();i++){
    if(tracks[i] != NULL){
    	Mix_FreeMusic(tracks[i]);
      tracks[i] = NULL;
    }
  }
  
  SDL_CloseAudio();
  SDL_DestroyWindow(window);
  
  TTF_CloseFont(font);
  IMG_Quit();
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();  
}

bool ResonanzShow::init(std::vector<std::string>& keywords, 
			std::vector<std::string>& pictures,
			std::vector<std::string>& sounds)
{
  this->keywords = keywords;
  this->pictures = pictures;
  this->sounds   = sounds;

  for(unsigned int i=0;i<images.size();i++)
    if(images[i] != NULL)
      SDL_FreeSurface(images[i]);

  for(unsigned int i=0;i<tracks.size();i++)
    if(tracks[i] != NULL)
      Mix_FreeMusic(tracks[i]);


  images.resize(pictures.size());
  
  for(unsigned int i=0;i<images.size();i++)
    images[i] = NULL;
  
  tracks.resize(sounds.size());

  for(unsigned int i=0;i<tracks.size();i++)
    tracks[i] = NULL;
  
  
  return true;  
}


bool ResonanzShow::showRandomScreen(unsigned int& keyword, unsigned int& picture)
{
  keyword = rand() % keywords.size();
  picture = rand() % pictures.size();
  
  return showScreen(keyword, picture);
}


bool ResonanzShow::showScreen(unsigned int keyword, unsigned int picture)
{
  SDL_Surface* surface = SDL_GetWindowSurface(window);
  SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
  
  int bgcolor = 0;
    
  
  if(picture < pictures.size()){ // shows a picture
    
    if(images[picture] == NULL){
      SDL_Surface* image = IMG_Load(pictures[picture].c_str());
      SDL_Rect imageRect;      
      SDL_Surface* scaled = 0;      	
      
      if(image != 0){
	if((image->w) > (image->h)){
	  double wscale = ((double)WIDTH)/((double)image->w);
	  scaled = zoomSurface(image, wscale, wscale, SMOOTHING_ON);
	}
	else{
	  double hscale = ((double)HEIGHT)/((double)image->h);
	  scaled = zoomSurface(image, hscale, hscale, SMOOTHING_ON);
	}
	
	images[picture] = scaled;
      }
      
      if(scaled != NULL){
	SDL_Color averageColor;
	measureColor(scaled, averageColor);
	bgcolor = (int)(averageColor.r + averageColor.g + averageColor.b)/3;
	
	imageRect.w = scaled->w;
	imageRect.h = scaled->h;
	imageRect.x = (WIDTH - scaled->w)/2;
	imageRect.y = (HEIGHT - scaled->h)/2;
	
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
      imageRect.x = (WIDTH - scaled->w)/2;
      imageRect.y = (HEIGHT - scaled->h)/2;
      
      SDL_BlitSurface(images[picture], NULL, surface, &imageRect);
    }
    
  }
  
  
  ///////////////////////////////////////////////////////////////////////
  
  if(keyword < keywords.size()){ // displays a text
    SDL_Color white = { 255, 255, 255 };
    SDL_Color red   = { 255, 0  , 0   };
    SDL_Color green = { 0  , 255, 0   };
    SDL_Color blue  = { 0  , 0  , 255 };
    SDL_Color black = { 0  , 0  , 0   };
    SDL_Color color;
    
    color = white;
    if(bgcolor > 160)
      color = black;
    
    SDL_Surface* msg = 
      TTF_RenderUTF8_Blended(font, keywords[keyword].c_str(), color);
    
    SDL_Rect messageRect;
    messageRect.x = (WIDTH - msg->w)/2;
    messageRect.y = (HEIGHT - msg->h)/2;
    messageRect.w = msg->w;
    messageRect.h = msg->h;
    
    SDL_BlitSurface(msg, NULL, surface, &messageRect);
    
    SDL_FreeSurface(msg);
  }
  

  SDL_UpdateWindowSurface(window);
  
  return true;
}


bool ResonanzShow::playRandomMusic(unsigned int& track)
{
  track = rand() % sounds.size();
  
  return playMusic(track);
}


bool ResonanzShow::playMusic(unsigned int track)
{
	Mix_FadeOutMusic(500);
  
  if(track < sounds.size()){
    
    if(tracks[track] == NULL)
      tracks[track] = Mix_LoadMUS(sounds[track].c_str());
    
    if(tracks[track] != NULL)
      Mix_FadeInMusic(tracks[track], -1, 500);
  }

  return track;
}


bool ResonanzShow::keypress()
{
  SDL_Event event;
  while(SDL_PollEvent(&event)){
    if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP){
      return true;
    }
  }
  
  return false;
}


void ResonanzShow::delay(unsigned int msecs)
{
  SDL_Delay(msecs);
}


bool ResonanzShow::measureColor(SDL_Surface* image, SDL_Color& averageColor)
{
  double r = 0.0;
  double g = 0.0;
  double b = 0.0;
  unsigned int N = 0;
  
  SDL_Surface* im =
    SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);
  
  if(im == 0) return false;
  
  unsigned int* buffer = (unsigned int*)im->pixels;
  
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
  
  r /= N;
  g /= N;
  b /= N;
  
  averageColor.r = (Uint8)r;
  averageColor.g = (Uint8)g;
  averageColor.b = (Uint8)b;

  SDL_FreeSurface(im);
  
  return true;
  
}



SDL_Surface* zoomSurface(SDL_Surface* image, double xscale, double yscale, int flag)
{
	// currently just copies surface and does nothing (TODO: do proper scaling)
	SDL_Surface* result = SDL_CreateRGBSurface(0, (int)(image->w*xscale), (int)(image->h*yscale), 32,
												0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	SDL_BlitScaled(image, NULL, result, NULL);



	return result;
}
