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
  
  virtual bool reset();
  
  virtual bool setParameters(const std::vector<float>& p);
  
  virtual int getNumberOfParameters();
  
 protected:
  double tbase;
  
  double A;  // amplitude/volume
  double Fc; // carrier frequency
  double Fm; // modulating frequency
  double d;  // delta of frequency
  
  virtual bool synthesize(int16_t* buffer, int samples);
  
};

#endif /* FMSOUNDSYNTHESIS_H_ */
