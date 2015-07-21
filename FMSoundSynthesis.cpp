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
  A  = 0.0;
  Fc = 440.0;
  Fm = 440.0;
  d  = 1.0;
  
  oldA  = 0.0;
  oldFc = 440.0;
  oldFm = 440.0;
  oldd  = 1.0;

  old_tbase = 0.0;
  tbase = 0.0;
  
  fadeoutTime = 100.0; // 100ms fade out between parameter changes
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
  // resets sound generation (timer)
  old_tbase = 0.0;
  tbase = 0.0;
  return true;
}


bool FMSoundSynthesis::getParameters(std::vector<float>& p)
{
  p.resize(4);
  
  p[0] = A;
  p[1] = Fc / 5000.0f;
  
  if(Fc > 0.0){
    p[2] = Fm/Fc; // harmonicity ratio
  }
  else{
    p[2] = 0.0;
  }
  
  if(Fm > 0.0){
    p[3] = d/Fm;  // modulation index;
  }
  else{
    p[3] = 0.0;
  }
  
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
  
  oldA  = A;
  oldFc = Fc;
  oldFm = Fm;
  oldd  = d;
  
  A  = p[0];

  // sound base frquency: [220, 1760 Hz] => note interval: A-3 - A-6
  float f = 440.0;
#if 0
  {
    // converts [0,1] to frequency interval [higher frequency notes more probable]
    f = 220.0 + (1760 - 220.0)*p[1];
    
    // converts to note value (440Hz centered equally tempered)
    int note = (int)round(12.0*log2(f/440.0)); // rounds to nearest note
    f = 440.0*pow(2.0, note/12.0);
  }
#else
  {
    // converts [0,1] to note [each note is equally probable]
    const unsigned int NOTES = 36; // note interval: (A-3 - A-6) [36 notes]
    int note = (int)round(p[1]*NOTES - 12.0); // note = 0 => A-4
    
    f = 440.0*pow(2.0, note/12.0); // converts note to frequency
  }
#endif
  
  
  Fc = f;
  
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
  
  // old_tbase = tbase;
  resetTime = getMilliseconds();
  
  // std::cout << "A  = " << A << std::endl;
  // std::cout << "Fc = " << Fc << std::endl;
  // std::cout << "Fm/Fc = " << Fm/Fc << std::endl;
  // std::cout << "Am/Fm = " << d/Fm << std::endl;
  
  return true;
}


int FMSoundSynthesis::getNumberOfParameters(){
  return 4;
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
  double hz = snd.freq;
  
  double timeSinceReset = (double)(getMilliseconds() - resetTime);
  
  for(int i=0;i<samples;i++){
    double dt = ((double)i)/hz;
    double t = tbase + dt;
    double old_t = old_tbase + dt;
    
    double F = (d*cos(Fm*t) + Fc);
    double value = A*cos(F*t);
  
    double oldF = (oldd*cos(oldFm*t) + oldFc);
    double old_value = oldA*cos(oldF*t);
    
    double now = timeSinceReset + dt/1000.0;
    
    if(now < fadeoutTime){
      double c = (now - resetTime)/fadeoutTime;
      value = c*value + (1.0 - c)*old_value;
    }
    
    buffer[i] = (int16_t)( value*(0x7FFF - 1) );
  }
  
  tbase += ((double)samples)/hz;
  old_tbase += ((double)samples)/hz;
  
  return true;
}


