/*
 * SDLSoundSynthesis.h
 *
 *  Created on: 15.2.2013
 *      Author: omistaja
 */

#ifndef SDLSOUNDSYNTHESIS_H_
#define SDLSOUNDSYNTHESIS_H_

#include <vector>
#include <string>
#include <stdint.h>
#include <SDL.h>

class SDLSoundSynthesis {
public:
  SDLSoundSynthesis();
  virtual ~SDLSoundSynthesis();
  
  bool play();
  bool pause();
  
  virtual std::string getSynthesizerName() = 0;
  
  virtual bool reset() = 0;
  
  virtual bool getParameters(std::vector<float>& p) = 0;
  
  virtual bool setParameters(const std::vector<float>& p) = 0;
  
  virtual int getNumberOfParameters() = 0;
  
 protected:
  SDL_AudioSpec snd;
  
  virtual bool synthesize(int16_t* buffer, int samples) = 0;
  
 private:
  SDL_AudioSpec desired;
  int open_status;
  
  friend void __sdl_soundsynthesis_mixaudio(void* unused, Uint8* stream, int len);
  
};

void __sdl_soundsynthesis_mixaudio(void* unused, Uint8* stream, int len);

#endif /* SDLSOUNDSYNTHESIS_H_ */
