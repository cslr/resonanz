
#include <stdio.h>
#include <iostream>
#include "FMSoundSynthesis.h"
// #include "FluidSynthSynthesis.h"
#include "SDLMicrophoneListener.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>


int main(int argc, char**argv)
{
  printf("Mini sound synthesis and capture test\n");

  bool useSDL = true;
  srand(time(0));

  // if(rand()&1) useSDL = true;

  SoundSynthesis* snd = NULL;
  SDLMicListener* mic = NULL;

  if(useSDL){
    SDL_Init(SDL_INIT_AUDIO);
    atexit(SDL_Quit);
    snd = new FMSoundSynthesis();
    // mic = new SDLMicListener();
  }
  else{
    // snd = new FluidSynthSynthesis("/usr/share/sounds/sf2/FluidR3_GM.sf2");
  }
  
  
  if(snd->play() == false){
    printf("Cannot start playback.\n");
    return -1;
  }

  if(mic){
    if(mic->listen() == false){
      printf("Cannot start audio capture.\n");
      return -1;
    }
  }
			   
  
  std::vector<float> p;
  p.resize(snd->getNumberOfParameters());
  
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

    // if(p.size() > 0) p[0] = 0.8;
      
    
    if(snd->setParameters(p) == false)
      std::cout << "set parameters failed." << std::endl;

    if(mic)
      std::cout << "SIGNAL POWER: " << mic->getInputPower() << std::endl;
    
    // snd->reset();
    
    SDL_Delay(1000);
  }

  if(snd) delete snd;
  if(mic) delete mic;
  
  SDL_Quit();
  
  return 0;
}
