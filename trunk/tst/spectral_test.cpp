/*
 * testing the spectral analysis and tranform of FFTW of EEG analysis
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>
#include "spectral_analysis.h"


int main(int argc, char** argv)
{
  // create test signals [200 Hz sampling], calculate its power spectrum
  // and print resulting coefficients (0-100 Hz) to file together with
  // hz values, use sinusoids as test signals
  
  srand(time(0));
  
  // TESTCASE 1: single sinusoid at 100Hz with 1000 sampling frequency
  // http://www.mathworks.se/help/signal/ug/psd-estimate-using-fft.html is a good reference test
  printf("TESTCASE1: spectral analysis of randomly chosen sinusoid.\n");
  
  {
    double Fs = 1000.0;
    std::vector<double> x;
    x.resize(1000);
    
    const double sin_hz =
      floor(10.0 + (((double)rand())/((double)RAND_MAX))*90.0); // 10-100 Hz
    double t = 0.0;
    
    double A = 10.0*(((double)rand())/((double)RAND_MAX));
    
    // samples sinusoid at sin_hz using sampling_hz 1000 Hz
    for(unsigned int i=0;i<x.size();i++){
      x[i] = A*cos(2*M_PI*sin_hz*t);
      t += 1.0/Fs;
    }
    
    std::vector<double> PF;
    double hz_per_sample = 0.0;
    
    if(power_spectral_analysis(x, Fs, PF, hz_per_sample) == false){
      fprintf(stderr, "ERROR: call to spectral_analysis() FAILED.\n");
      return -1;
    }
    
    // finds the largest coefficient
    double largest_hz_value = 0.0;
    double largest_hz_hz    = 0.0;
    double hz = 0.0;
    
    printf("length(PF) = %d\n", PF.size());
    
    for(unsigned int i=0;i<PF.size();i++){
      if(PF[i] > largest_hz_value){
	largest_hz_value = PF[i];
	largest_hz_hz = hz;
      }
      
      hz += hz_per_sample;
    }
    
    printf("Generated sinusoid at %f Hz (estimated power peak: %f)\n", 
	   sin_hz, A*A/2.0);
    printf("SPECTRAL ANALYSIS: Largest coefficient at %f Hz: %f\n", 
	   largest_hz_hz, largest_hz_value);
    fflush(stdout);
    
  }
  

  
	  
  
  // TESTCASE 1: single sinusoid at 100Hz with 1000 sampling frequency
  // http://www.mathworks.se/help/signal/ug/psd-estimate-using-fft.html is a good reference test
  printf("TESTCASE2: spectral analysis of delta, gamma, theta, alpha-bands..\n");  
  {
    fprintf(stderr, "ERROR: spectral_analysis() test function not implemented\n");
  }
  
  
  return 0;
}
