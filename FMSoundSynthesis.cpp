/*
 * FMSoundSynthesis.cpp
 *
 */

#include "FMSoundSynthesis.h"
#include <iostream>
#include <chrono>
#include <math.h>


FMSoundSynthesis::FMSoundSynthesis() {
  
  // default parameters: silence
  std::vector<float> p;
  p.resize(4);
  p[0] = 0.0f;
  p[1] = 0.0f;
  p[2] = 0.0f;
  p[3] = 0.0f;
  
  currentp.resize(4);
  currentp[0] = 0.0f;
  currentp[1] = 0.0f;
  currentp[2] = 0.0f;
  currentp[3] = 0.0f;
  
  setParameters(p);
  setParameters(p);
  
  tbase = 0.0;
  
  fadeoutTime = 1000.0; // 1000ms fade out between parameter changes
}


FMSoundSynthesis::~FMSoundSynthesis() {
  // TODO Auto-generated destructor stub
}


std::string FMSoundSynthesis::getSynthesizerName()
{
  return "SDL FM Sound Synthesis (Space noise)";
}

bool FMSoundSynthesis::reset()
{
  // resets sound generation (timer)
  tbase = 0.0;
  return true;
}


bool FMSoundSynthesis::getParameters(std::vector<float>& p)
{
  p = currentp;
  
  return true;
}


bool FMSoundSynthesis::setParameters(const std::vector<float>& p_)
{
  auto p = p_;
  
  if(p.size() != 4) return false;
  
  currentp = p; // copies values for getParameters()
  
  // limit parameters to [0,1] range
  for(unsigned int i=0;i<p.size();i++){
    if(p[i] < 0.0f) p[i] = 0.0f;
    if(p[i] > 1.0f) p[i] = 1.0f;
  }
  
  oldAc = Ac;
  oldFc = Fc;
  oldFm = Fm;
  oldAm = Am;
  
  Ac = p[0];
  
  // sound base frquency: [220, 1760 Hz] => note interval: A-3 - A-6
  float f = 440.0;
  {
    // converts [0,1] to note [each note is equally probable]
    const float NOTES = 24.9999; // note interval: (A-3 - A-5) [24 notes]
    double note = floor(p[1]*NOTES) - 12.0; // note = 0 => A-4
    
    // printf("note = %f\n", note); fflush(stdout);
    
    f = 440.0*pow(2.0, note/12.0); // converts note to frequency
  }
  
  
  Fc = f;
  
  {
    // harmonicity ratio: Fm/Fc [0,1] => [0,3] values possible
    float h = floor(p[2]*3.999999);
    
    // printf("harmonicity = %f\n", h); fflush(stdout);
    
    h = round(h)*Fc; // [0, 1, 2, 3]
    
    Fm = h;
  }
  
  {
    // modulation index: Am/Fm
    float m = p[3];
    
    m = 0.005*p[3]*p[3]*p[3]*p[3];
    
    Am = m*Fm;
  }
  
  resetTime = getMilliseconds();
  // tbase = 0.0;
  
  // std::cout << "A  = " << A << std::endl;
  // std::cout << "Fc = " << Fc << std::endl;
  // std::cout << "Fm/Fc = " << Fm/Fc << std::endl;
  // std::cout << "Am/Fm = " << d/Fm << std::endl;
  
  return true;
}


int FMSoundSynthesis::getNumberOfParameters(){
  return 4;
}


unsigned long long FMSoundSynthesis::getSoundSynthesisSpeedMS()
{
  return fadeoutTime;
}


// milliseconds since epoch
unsigned long long FMSoundSynthesis::getMilliseconds()
{
  auto t1 = std::chrono::system_clock::now().time_since_epoch();
  auto t1ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
  return (unsigned long long)t1ms;
}


bool FMSoundSynthesis::synthesize(int16_t* buffer, int samples)
{
  double hz = (double)snd.freq;
  
  double timeSinceReset = (double)(getMilliseconds() - resetTime);
  
  for(int i=0;i<samples;i++){
    const double dt = ((double)i)/hz;
    const double t = tbase + dt;
    
    const double now = timeSinceReset + dt*1000.0;
    
    double Fc_ = Fc;
    double oldFc_ = oldFc;
    
    double Fm_ = Fm;
    double oldFm_ = oldFm;
    
    double Am_ = Am;
    double oldAm_ = oldAm;
    
    if(now < fadeoutTime){
      double c = now/fadeoutTime;
      double f = c*Fc_ + (1.0 - c)*oldFc_;
      Fc_ = f;
      oldFc_ = f;

#if 1
      double fm = c*Fm_ + (1.0 - c)*oldFm_;
      Fm_ = fm;
      oldFm_ = fm;

      double am = c*Am_ + (1.0 - c)*oldAm_;
      Am_ = am;
      oldAm_ = am;
#endif
    }
    
    double F = (Am_*cos(2.0*M_PI*Fm_*t) + Fc_);
    double value = Ac*cos(2.0*M_PI*F*t);
    
    double oldF = (oldAm_*cos(2.0*M_PI*oldFm_*t) + oldFc_);
    double oldvalue = oldAc*cos(2.0*M_PI*oldF*t);

    if(now < fadeoutTime){
      double c = now/fadeoutTime;
      value  = c*value  + (1.0 - c)*oldvalue;
    }
    
    buffer[i] = (int16_t)( value*32767 );
  }
  
  tbase += ((double)samples)/hz;
  
  return true;
}


