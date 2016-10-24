// Tomas Ukkonen 10/2016

#ifndef SDLMICLISTERNER_H_
#define SDLMICLISTERNER_H_

#include <vector>
#include <string>
#include <stdint.h>
#include <SDL.h>


class SDLMicListener {
public:

  SDLMicListener();
  virtual ~SDLMicListener();

  bool listen(); // start listening mic

  // returns signal power in DECIBELs
  // (max level is 0 and values are negative)
  virtual double getInputPower();

protected:
  SDL_AudioSpec snd;

  virtual bool listener(int16_t* buffer, int samples);

private:
  SDL_AudioDeviceID dev;
  SDL_AudioSpec desired;

  double currentPower = 0.0;

  friend void __sdl_soundcapture(void* unused, Uint8* stream, int len);
  
};

void __sdl_soundcapture(void* unused, Uint8* stream, int len);

#endif
