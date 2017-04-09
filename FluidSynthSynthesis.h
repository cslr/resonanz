/*
 * Fluidsynth MIDI sound synthesizer
 *
 * (C) Copyright Tomas Ukkonen 2017
 */

#ifndef FLUIDSYNTHSYNTHESIS_H
#define FLUIDSYNTHSYNTHESIS_H

#include <vector>
#include <string>
#include <stdint.h>

#include <mutex>

#include <fluidsynth.h>

#include "SoundSynthesis.h"


class FluidSynthSynthesis : public SoundSynthesis
{
 public:
  
  FluidSynthSynthesis(const std::string& soundfont);
  virtual ~FluidSynthSynthesis();
  
  virtual bool play();
  virtual bool pause();

  virtual std::string getSynthesizerName();
    
  virtual bool reset();
    
  virtual bool getParameters(std::vector<float>& p);
    
  virtual bool setParameters(const std::vector<float>& p);
    
  virtual int getNumberOfParameters();

 protected:

  std::mutex synth_mutex;
  fluid_settings_t* settings;
  fluid_synth_t* synth;
  fluid_audio_driver_t* adriver;
  int soundfont;

  std::vector<float> lastParameters;

  int prev_channel;
  int prev_key;
  
};

#endif

