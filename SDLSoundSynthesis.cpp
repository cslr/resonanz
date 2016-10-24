/*
 * SDLSoundSynthesis.cpp
 *
 *  Created on: 15.2.2013
 *      Author: omistaja
 */

#include "SDLSoundSynthesis.h"

SDLSoundSynthesis::SDLSoundSynthesis()
{
  SDL_zero(desired);
  SDL_zero(snd);
  
  desired.freq = 44100;
  desired.format = AUDIO_S16SYS;
  desired.channels = 1; // use mono sound for now
  desired.samples = 4096;
  desired.callback = __sdl_soundsynthesis_mixaudio;
  desired.userdata = this;
  
  dev = 0;
  
}

SDLSoundSynthesis::~SDLSoundSynthesis() {
  if(dev != 0){
    SDL_CloseAudioDevice(dev);
    dev = 0;
  }
  
}


bool SDLSoundSynthesis::play()
{
  if(dev == 0){
    dev = SDL_OpenAudioDevice(NULL, 0, &desired, &snd, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  }
  
  if(dev == 0){
    printf("SDL Error: %s\n", SDL_GetError());
    
    // prints audio devices
    const int numDevices = SDL_GetNumAudioDevices(0);
    if(numDevices >= 0)
      printf("Number of audio devices: %ḍn", numDevices);
    
    for(int i=0;i<numDevices;i++){
      printf("Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }
    
    return false;
  }
  
  if(snd.format != AUDIO_S16SYS || snd.channels != 1){
    SDL_CloseAudioDevice(dev);
    dev = 0;
    return false;
  }
  
  SDL_PauseAudioDevice(dev, 0);
  
  return true;
}


bool SDLSoundSynthesis::pause()
{
  if(dev != 0)
    SDL_PauseAudioDevice(dev, 1);
  else
    return false;
  
  return true;
}


void __sdl_soundsynthesis_mixaudio(void* unused, 
				   Uint8* stream, int len)
{
  SDLSoundSynthesis* s = (SDLSoundSynthesis*)unused;
  
  if(s == NULL) return;
  
  s->synthesize((int16_t*)stream, len/2);
}

