
#include <stdio.h>
#include <iostream>
#include "FMSoundSynthesis.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char**argv)
{
  printf("Mini FM sound synthesis test\n");
  
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
	p[i] = p[i] + 0.1f*(v - 0.5f);
	if(p[i] < 0.0f)
	  p[i] = 0.0f;
	else if(p[i] > 1.0f)
	  p[i] = 1.0f;
      }
      while(p[i] <= 0.0f);
    }
    
    p[3] = 1.0f;
    
    if(snd.setParameters(p) == false)
      std::cout << "set parameters failed." << std::endl;
    
    snd.reset();
    
    sleep(1);
  }
  
  
  return 0;
}
