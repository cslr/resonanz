
#include "FluidSynthSynthesis.h"

#include <string>
#include <math.h>


FluidSynthSynthesis::FluidSynthSynthesis(const std::string& soundfont_file)
{
  settings = NULL;
  synth = NULL;
  adriver = NULL;
  soundfont = FLUID_FAILED;
  
  {
    std::unique_lock<std::mutex> lock(synth_mutex);

    settings = new_fluid_settings();

    // use pulseaudio audio driver
    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    fluid_settings_setint(settings, "synth.midi-channels", 60);
    fluid_settings_setnum(settings, "synth.gain", 4.0);
    
    synth    = new_fluid_synth(settings);
    adriver  = new_fluid_audio_driver(settings, synth);
    
    if(settings == NULL || synth == NULL || adriver == NULL){
      if(adriver){ delete_fluid_audio_driver(adriver); adriver = NULL; }
      if(synth){ delete_fluid_synth(synth); synth = NULL; }
      if(settings){ delete_fluid_settings(settings); settings = NULL; }
    }

    if(synth){
      soundfont = fluid_synth_sfload(synth, soundfont_file.c_str(), 1);
      
      if(soundfont == FLUID_FAILED){
	printf("WARN: loading soundfont failed\n");
      }
      else{
	// selects separate soundfont for each channel
	for(unsigned int i=0;i<60;i++){
	  fluid_synth_program_select(synth, i, soundfont, 0, i); // BANK 0 PRESET i
	}
      }
	
    }

    lastParameters.resize(3);

    prev_channel = 0;
    prev_key = 0;
  }
}


FluidSynthSynthesis::~FluidSynthSynthesis()
{
  {
    std::unique_lock<std::mutex> lock(synth_mutex);

    if(adriver){ delete_fluid_audio_driver(adriver); adriver = NULL; }

    if(soundfont != FLUID_FAILED)
      fluid_synth_sfunload(synth, soundfont, 1);
    
    if(synth){ delete_fluid_synth(synth); synth = NULL; }
    if(settings){ delete_fluid_settings(settings); settings = NULL; }
  }
  
}

bool FluidSynthSynthesis::play()
{
  return true;
}


bool FluidSynthSynthesis::pause()
{
  return true;
}

std::string FluidSynthSynthesis::getSynthesizerName()
{
  return "fluidsynth";
}

bool FluidSynthSynthesis::reset()
{
  return true;
}


bool FluidSynthSynthesis::getParameters(std::vector<float>& p)
{
  p = lastParameters;
  return true;
}

bool FluidSynthSynthesis::setParameters(const std::vector<float>& p)
{
  if(p.size() == 3)
  {
    std::unique_lock<std::mutex> lock(synth_mutex);

    int channel  = 0;
    int key      = 60;
    int velocity = 100;

    float ch = ((p[0] < 0.0f) ? 0.0f : ((p[0] > 1.0) ? 1.0f : p[0]));
    float k  = ((p[1] < 0.0f) ? 0.0f : ((p[1] > 1.0) ? 1.0f : p[1]));
    float v  = ((p[2] < 0.0f) ? 0.0f : ((p[2] > 1.0) ? 1.0f : p[2]));

    channel  = floor(ch*60.9999); // [0,60]
    key = 48 + floor(k*35.99999); // C-4 - B-6 (3 octaves)
    velocity = floor(v*127.9999); // [0,127]

    channel = 16;

    
    if(synth){
      fluid_synth_noteoff(synth, prev_channel, prev_key);

      fluid_synth_noteon(synth, channel, key, velocity);

      printf("NOTE CH %d KEY %d VEL %d\n", channel, key, velocity);

      prev_channel = channel;
      prev_key = key;
    }

    lastParameters = p;
  }
			 
  
  return true;
}

int FluidSynthSynthesis::getNumberOfParameters()
{
  return 3;
}
