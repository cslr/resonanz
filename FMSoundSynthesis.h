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

  virtual double getSynthPower();
  
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
  
  static const unsigned int NBUFFERS = 10;
  std::vector<int16_t> prevbuffer[NBUFFERS];
  
  virtual bool synthesize(int16_t* buffer, int samples);


  double currentPower = 0.0; // current output signal power
  
};

#endif /* FMSOUNDSYNTHESIS_H_ */
