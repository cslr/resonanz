/*
 * FMSoundSynthesis.cpp
 *
 */

#include "FMSoundSynthesis.h"
#include <iostream>
#include <math.h>


FMSoundSynthesis::FMSoundSynthesis() {
  
  // default parameters: silence
  A  = 0.0;
  Fc = 440.0;
  Fm = 440.0;
  d  = 0.0;
  
  tbase = 0.0;
}


FMSoundSynthesis::~FMSoundSynthesis() {
  // TODO Auto-generated destructor stub
}


std::string FMSoundSynthesis::getSynthesizerName()
{
  return "SDL FM Sound Synthesis";
}

bool FMSoundSynthesis::reset()
{
  // there is minor sound synthesis glitch possibility
  tbase = 0.0;
  return true;
}


bool FMSoundSynthesis::getParameters(std::vector<float>& p)
{
  p.resize(4);
  
  p[0] = A;
  p[1] = Fc / 5000.0f;
  p[2] = Fm/Fc; // harmonicity ratio
  p[3] = d/Fm;  // modulation index;
  
  return true;
}


bool FMSoundSynthesis::setParameters(const std::vector<float>& p_)
{
  auto p = p_;
  
  if(p.size() != 4) return false;
  
  // limit parameters to [0,1] range
  for(unsigned int i=0;i<p.size();i++){
    if(p[i] < 0.0f) p[i] = 0.0f;
    if(p[i] > 1.0f) p[i] = 1.0f;
  }
  
  A  = p[0];

  Fc = p[1]*5000.0; // sound base frquency: [0, 5 kHz]
  
  {
    // harmonicity ratio: Fm/Fc [0,1] => [1,4] values possible
    float h = (1.0 + p[2]*3.0);
    h = round(h)*Fc; // [1, 2, 3, 4]
    
    Fm = h;
  }
  
  {
    // modulation index: Am/Fm
    float m = p[3];
    
    m = 0.1*p[3];
    
    d = m*Fm;
  }
  
  // std::cout << "A  = " << A << std::endl;
  // std::cout << "Fc = " << Fc << std::endl;
  // std::cout << "Fm/Fc = " << Fm/Fc << std::endl;
  // std::cout << "Am/Fm = " << d/Fm << std::endl;
  
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


