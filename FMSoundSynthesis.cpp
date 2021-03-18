/*
 * FMSoundSynthesis.cpp
 *
 */

#include "FMSoundSynthesis.h"
#include <iostream>
#include <chrono>
#include <math.h>
#include <vector>

#ifndef M_PI
#define M_PI 3.141592653
#endif

FMSoundSynthesis::FMSoundSynthesis() {
  
  // default parameters: silence
  currentp.resize(3);
  
  for(auto& pi : currentp)
    pi = 0.0f;
  
  Ac = 0.0;
  Fc = 0.0;
  Fm = 0.0;
  Am = 0.0;
  
  oldAc = 0.0;
  oldFc = 0.0;
  oldFm = 0.0;
  oldAm = 0.0;
  
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
  
  if(p.size() != 3) return false;
  
  // limit parameters to [0,1] range
  for(auto& pi : p){
    if(pi < 0.0f) pi = 0.0f;
    if(pi > 1.0f) pi = 1.0f;
  }
  
  currentp = p; // copies values for getParameters()
  
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
    // float h = floor(p[2]*3.999999);
    
    float h = 1.0f;
    h = round(h)*Fc; // [0, 1, 2, 3]
    
    Fm = h;
  }
  
  {
    // modulation index: Am/Fm
    float m = p[2];
    
    m = (10e-7)*p[2]*p[2]; // sets maximum value explicitly
    
    Am = m*Fm;
  }
  
  resetTime = getMilliseconds();
  // tbase = 0.0;
  
  // std::cout << "Ac  = " << Ac << std::endl;
  // std::cout << "Fc = " << Fc << std::endl;
  // std::cout << "Fm/Fc = " << Fm/Fc << std::endl;
  // std::cout << "Am/Fm = " << Am/Fm << std::endl;
  
  return true;
}


int FMSoundSynthesis::getNumberOfParameters(){
  return 3;
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


double FMSoundSynthesis::getSynthPower()
{
  // returns values as negative decibels
  double Pref = 32767.0*32767.0;
  double dbel = 10.0*log10(currentPower/Pref);
  return dbel;
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
    
    double delayF = Fc_;
    
    if(now < fadeoutTime){
      double c = now/fadeoutTime;
      double f = c*Fc_ + (1.0 - c)*oldFc_;
      delayF;
      // Fc_ = f;
      // oldFc_ = f;

      double fm = c*Fm_ + (1.0 - c)*oldFm_;
      Fm_ = fm;
      oldFm_ = fm;

      double am = c*Am_ + (1.0 - c)*oldAm_;
      Am_ = am;
      oldAm_ = am;
    }
    
    double F = (Am_*cos(2.0*M_PI*Fm_*t) + Fc_);
    double value = Ac*cos(2.0*M_PI*F*t);

    double oldF = (oldAm_*cos(2.0*M_PI*oldFm_*t) + oldFc_);
    double oldvalue = oldAc*cos(2.0*M_PI*oldF*t);
    
    double NOTE_FADEOUT_TIME = 1000.0;

    if(now < NOTE_FADEOUT_TIME){
      double c = now/NOTE_FADEOUT_TIME;
      double r = (1.0 - c);
      value  = value  + r*r*r*r*oldvalue;
    }
    
    // delay effect
    {
      std::vector<double> delay;
      std::vector<double> delayA;
      
      // delay.push_back(25.3/delayF);
      // delay.push_back(50.7/delayF);
      // delay.push_back(75.0/delayF);
      
      delay.push_back(100/1000.0);
      delay.push_back(200/1000.0);
      delay.push_back(300/1000.0);
      delayA.push_back(-0.50);
      delayA.push_back(+0.05);
      delayA.push_back(-0.01);
      
      
      for(unsigned int efx=0;efx<delay.size();efx++){
	int delaysamples = int(hz*delay[efx]);
	int index = i - delaysamples;
	
	if(index >= 0)
	  value += delayA[efx]*buffer[i - delaysamples]/32767.0;
	else{
	  for(unsigned int b=0;b<NBUFFERS;b++){
	    index += prevbuffer[b].size();
	    if(index >= 0 && index < prevbuffer[b].size()){
	      value += delayA[efx]*prevbuffer[b][index]/32767.0;
	      break;
	    }
	  }
	}
      }
    }
    
    if(value <= -1.0) value = -1.0;
    else if(value >= 1.0) value = 1.0;
    
    value = value; // non-sinusoid..
    
    buffer[i] = (int16_t)( value*32767 );
  }
  
  for(int i=(NBUFFERS-2);i>=0;i--){ // should swap pointers instead..    
    prevbuffer[i+1] = prevbuffer[i];
  }
  
  prevbuffer[0].resize(samples);

  double power;

  for(int i=0;i<samples;i++){
    prevbuffer[0][i] = buffer[i];

    power += ((double)buffer[i])*((double)buffer[i])/((double)samples);
    
#if 0
    // does simple low pass filtering take sum of recent samples
    int sum = 0;
    for(int j=0;j<10;j++){
      if(i-j >= 0){
	sum += ((int)buffer[i-j]);
      }
      else{
	int index = i-j+prevbuffer[1].size();
	
	if(index >= 0){
	  sum += (int)prevbuffer[1][index];
	}
      }
    }
    sum /= 10;
    
    prevbuffer[0][i] = sum;
#endif
  }

  currentPower = power;
  
  tbase += ((double)samples)/hz;
  
  return true;
}


