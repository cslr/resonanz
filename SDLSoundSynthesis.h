/*
 * SDLSoundSynthesis.h
 *
 *  Created on: 15.2.2013
 *      Author: Tomas Ukkonen
 */

#ifndef SDLSOUNDSYNTHESIS_H_
#define SDLSOUNDSYNTHESIS_H_

#include <vector>
#include <string>
#include <stdint.h>
#include <SDL.h>

#include "SoundSynthesis.h"

class SDLSoundSynthesis : public SoundSynthesis
{
public:
  SDLSoundSynthesis();
  virtual ~SDLSoundSynthesis();
  
  virtual bool play();
  virtual bool pause();

  /*
    virtual std::string getSynthesizerName() = 0;
    
    virtual bool reset() = 0;
    
    virtual bool getParameters(std::vector<float>& p) = 0;
    
    virtual bool setParameters(const std::vector<float>& p) = 0;
    
    virtual int getNumberOfParameters() = 0;
  */
  
  // recommended time it takes to synthesize sound: 
  // it is not good idea to change parameters faster than this
  virtual unsigned long long getSoundSynthesisSpeedMS() = 0;

  // return current signal power of synthesized sound in DECIBELs
  virtual double getSynthPower() = 0; 
  
 protected:  
  SDL_AudioSpec snd;
  
  virtual bool synthesize(int16_t* buffer, int samples) = 0;
  
 private:
  SDL_AudioDeviceID dev;
  SDL_AudioSpec desired;
  
  friend void __sdl_soundsynthesis_mixaudio(void* unused, Uint8* stream, int len);
  
};

void __sdl_soundsynthesis_mixaudio(void* unused, Uint8* stream, int len);

#endif /* SDLSOUNDSYNTHESIS_H_ */
