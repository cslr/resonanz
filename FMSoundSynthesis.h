/*
 * FMSoundSynthesis.h
 *
 */

#ifndef FMSOUNDSYNTHESIS_H_
#define FMSOUNDSYNTHESIS_H_

#include "SDLSoundSynthesis.h"
#include <vector>


class FMSoundSynthesis: public SDLSoundSynthesis {
public:
  FMSoundSynthesis();
  virtual ~FMSoundSynthesis();
  
  virtual std::string getSynthesizerName();
  
  virtual bool reset();
  
  virtual bool getParameters(std::vector<float>& p);
  
  virtual bool setParameters(const std::vector<float>& p);
  
  virtual int getNumberOfParameters();
  
  virtual unsigned long long getSoundSynthesisSpeedMS();
  
 protected:
  // milliseconds since epoch
  unsigned long long getMilliseconds();
  
  double tbase;
  
  double Ac; // amplitude/volume of carrier
  double Fc; // carrier frequency
  double Fm; // modulating frequency
  double Am; // amplitude of modulator: delta of frequency
  
  std::vector<float> currentp;
  
  unsigned long long resetTime = 0ULL;
  double fadeoutTime;
  
  double oldAc;
  double oldFc;
  double oldFm;
  double oldAm;
  
  virtual bool synthesize(int16_t* buffer, int samples);
  
};

#endif /* FMSOUNDSYNTHESIS_H_ */
