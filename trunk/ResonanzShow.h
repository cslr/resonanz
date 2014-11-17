
#include <vector>
#include <string>

#ifndef __resonanz_show
#define __resonanz_show

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL2_rotozoom.h>
#include <SDL_mixer.h>



class ResonanzShow
{
 public:
  ResonanzShow(int WIDTH, int HEIGHT);
  ~ResonanzShow();
  
  bool init(std::vector<std::string>& keywords, 
	    std::vector<std::string>& pictures,
	    std::vector<std::string>& sounds);
  
  bool showRandomScreen(unsigned int& keyword, unsigned int& picture);
  bool showScreen(unsigned int keyword, unsigned int picture);

  bool playRandomMusic(unsigned int& track);
  bool playMusic(unsigned int track);
  
  bool keypress();
  
  void delay(unsigned int msecs);
  
 private:
  bool measureColor(SDL_Surface* image, SDL_Color& averageColor);
  
  
  std::vector<std::string> keywords;
  std::vector<std::string> pictures;
  std::vector<std::string> sounds;
  
  TTF_Font* font;
  
  int WIDTH;
  int HEIGHT;
  
  SDL_Window* window;
  
  std::vector<SDL_Surface*> images;
  std::vector<Mix_Music*>   tracks;
  
};


#endif
