
#include "SDLMicrophoneListener.h"
#include <math.h>

SDLMicListener::SDLMicListener()
{
  SDL_zero(desired);
  SDL_zero(snd);

  desired.freq = 44100;
  desired.format = AUDIO_S16SYS;
  desired.channels = 1; // use mono sound for now
  desired.samples = 4096;
  desired.callback = __sdl_soundcapture;
  desired.userdata = this;

  dev = 0;
  currentPower = 0.0;
}


SDLMicListener::~SDLMicListener()
{
  if(dev != 0){
    SDL_CloseAudioDevice(dev);
    dev = 0;
    currentPower = 0.0;
  }
}


// start listening mic
bool SDLMicListener::listen()
{
  if(dev == 0){
    dev = SDL_OpenAudioDevice(NULL, 1, &desired, &snd,
			      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  }

  if(dev == 0){
    printf("SDL Error: %s\n", SDL_GetError());
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


// returns signal power in DECIBELs
// (max level is 0 and values are negative)
double SDLMicListener::getInputPower()
{
  // returns values as negative decibels
  double Pref = 32767.0*32767.0;
  double dbel = 10.0*(log(currentPower/Pref)/log(10.0));
  return dbel;
}


bool SDLMicListener::listener(int16_t* buffer, int samples)
{
  // calculates current power
  double power = 0.0;
  
  for(int i=0;i<samples;i++){
    power += ((double)buffer[i])*((double)buffer[i])/((double)samples);
  }

  currentPower = power;
  
  return true;
}


void __sdl_soundcapture(void* unused, Uint8* stream, int len)
{
  SDLMicListener* s = (SDLMicListener*)unused;
  
  if(s == NULL) return;
  
  s->listener((int16_t*)stream, len/2);
}

