/*
 * FMSoundSynthesis.h
 *
 */

#ifndef FMSOUNDSYNTHESIS_H_
#define FMSOUNDSYNTHESIS_H_

#include "SDLSoundSynthesis.h"

class FMSoundSynthesis: public SDLSoundSynthesis {
public:
  FMSoundSynthesis();
  virtual ~FMSoundSynthesis();
  
  virtual std::string getSynthesizerName();
  
  virtual bool reset();
  
  virtual bool getParameters(std::vector<float>& p);
  
  virtual bool setParameters(const std::vector<float>& p);
  
  virtual int getNumberOfParameters();
  
 protected:
  // milliseconds since epoch
  unsigned long long getMilliseconds();
  
  
  double tbase;
  double old_tbase;
  
  double A;  // amplitude/volume
  double Fc; // carrier frequency
  double Fm; // modulating frequency
  double d;  // delta of frequency
  
  unsigned long long resetTime;
  double fadeoutTime;
  
  double oldA;
  double oldFc;
  double oldFm;
  double oldd;
  
  virtual bool synthesize(int16_t* buffer, int samples);
  
};

#endif /* FMSOUNDSYNTHESIS_H_ */
