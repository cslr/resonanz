/*
 * Generic Sound Synthesis interface class
 *
 * used by SDLSoundSynthesis and FluidSynthSoundSynthesis
 *
 * (C) Copyright Tomas Ukkonen 2017
 */

#ifndef SOUNDSYNTHESIS_H_
#define SOUNDSYNTHESIS_H_

#include <vector>
#include <string>
#include <stdint.h>

class SoundSynthesis {
 public:
  
  SoundSynthesis();
  virtual ~SoundSynthesis();

  virtual bool play() = 0;
  virtual bool pause() = 0;
  
  virtual std::string getSynthesizerName() = 0;
  
  virtual bool reset() = 0;
  
  virtual bool getParameters(std::vector<float>& p) = 0;
  
  virtual bool setParameters(const std::vector<float>& p) = 0;
  
  virtual int getNumberOfParameters() = 0;
  
};


#endif
