/*
 * SDLSoundSynthesis.cpp
 *
 *  Created on: 15.2.2013
 *      Author: omistaja
 */

#include "SDLSoundSynthesis.h"

SDLSoundSynthesis::SDLSoundSynthesis()
{
  desired.freq = 48000;
  desired.format = AUDIO_S16SYS;
  desired.channels = 1; // use mono sound for now
  desired.samples = 2048;
  desired.callback = __sdl_soundsynthesis_mixaudio;
  desired.userdata = this;
  
  open_status = -1;
  
}

SDLSoundSynthesis::~SDLSoundSynthesis() {
  if(open_status >= 0)
    SDL_CloseAudio();
  
  open_status = -1;
}


bool SDLSoundSynthesis::play()
{
  if(open_status < 0){
    open_status = SDL_OpenAudio(&desired, &snd);
  }
  
  if(open_status < 0)
    return false;
  
  if(snd.format != AUDIO_S16SYS || snd.channels != 1){
    SDL_CloseAudio();
    open_status = -1;
    return false;
  }
  
  
  SDL_PauseAudio(0);
  
  return true;
}


bool SDLSoundSynthesis::pause()
{
  if(open_status >= 0)
    SDL_PauseAudio(1);
  
  return true;
}


void __sdl_soundsynthesis_mixaudio(void* unused, 
				   Uint8* stream, int len)
{
  SDLSoundSynthesis* s = (SDLSoundSynthesis*)unused;
  
  if(s == NULL) return;
  
  s->synthesize((int16_t*)stream, len/2);
}

