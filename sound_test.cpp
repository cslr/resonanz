
#include <stdio.h>
#include <iostream>
#include "FMSoundSynthesis.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>

int main(int argc, char**argv)
{
  printf("Mini FM sound synthesis test\n");
  
  SDL_Init(SDL_INIT_AUDIO);
  atexit(SDL_Quit);
  
  srand(time(0));
  
  FMSoundSynthesis snd;
  
  if(snd.play() == false){
    printf("Cannot start playback.\n");
    return -1;
  }
  
  std::vector<float> p;
  p.resize(snd.getNumberOfParameters());
  
  for(unsigned int i=0;i<p.size();i++)
    p[i] = ((float)rand()) / ((float)RAND_MAX);
  
  while(1){
    for(unsigned int i=0;i<p.size();i++){
      do {
	float v = ((float)rand()) / ((float)RAND_MAX);
	p[i] = v;
	
	if(p[i] < 0.0f)
	  p[i] = 0.0f;
	else if(p[i] > 1.0f)
	  p[i] = 1.0f;
      }
      while(p[i] <= 0.0f);
    }

    p[0] = 0.8;
    
    if(snd.setParameters(p) == false)
      std::cout << "set parameters failed." << std::endl;
    
    // snd.reset();
    
    SDL_Delay(1000);
  }
  
  SDL_Quit();
  
  return 0;
}
