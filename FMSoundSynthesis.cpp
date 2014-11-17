/*
 * FMSoundSynthesis.cpp
 *
 */

#include "FMSoundSynthesis.h"
#include <math.h>

FMSoundSynthesis::FMSoundSynthesis() {
  // TODO Auto-generated constructor stub
  
  A  = 0.5;
  Fc = 440.0;
  Fm = 280.0;
  d  = 10.0;
  
  tbase = 0.0;
}


FMSoundSynthesis::~FMSoundSynthesis() {
  // TODO Auto-generated destructor stub
}


bool FMSoundSynthesis::reset()
{
  // there is minor sound synthesis glitch possibility
  tbase = 0.0;
  return true;
}


bool FMSoundSynthesis::setParameters(const std::vector<float>& p)
{
  if(p.size() != 4) return false;
  
  A  = p[0];
  Fc = p[1];
  Fm = p[2];
  d  = p[3];
  
  return true;
}


int FMSoundSynthesis::getNumberOfParameters(){
  return 4;
}


bool FMSoundSynthesis::synthesize(int16_t* buffer, int samples)
{
  double hz = snd.freq;
  
  for(int i=0;i<samples;i++){
    double t = tbase + ((double)i)/hz;
    double F = (d*cos(Fm*t) + Fc);
    
    buffer[i] = (int16_t)( A*cos( F*t )*(0x7FFF - 1) );
  }
  
  tbase += ((double)samples)/hz;
  
  return true;
}


