/*
 * Calculates spectral analysis of real valued 1d signal data (EEG)
 * This can be then used to calculate power specturum of the data
 * and to estimate power in EEG bands: 
 *
 * delta 0-4    Hz (slow wave sleep)
 * theta 4-8    Hz (drowsiness / idling)
 * alpha 8-16   Hz (relaxed)
 * beta  16-32  Hz (alert, active)
 * gamma 32-100 Hz (cogntively active)
 * mu    8-12   Hz (sensimotor cortex)
 *
 * Power is normalized as P = sum(|FFT[lo_hz .. hi_hz}|) / (hi_hz - lo_hz)
 */ 

#include "spectral_analysis.h"
#include <vector>
#include <fftw3.h> // GPL
#include <math.h>


// returns ||FFT(s)||^2
bool power_spectral_analysis(const std::vector<double>& s, const double sampling_hz,
			     std::vector<double>& PF, double& hz_per_sample)
{
  // FIXME: we do not care about speed but simplicity so we calculate 
  // plan each time when using FFTW3
  
  if(s.size() <= 0 || sampling_hz <= 0.0)
    return false;
  
  const unsigned int N = s.size();
  
  fftw_plan p;
  fftw_complex *out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
  
  if(out == NULL)
    return false;
  
  // warning we ignore const in input argument
  p = fftw_plan_dft_r2c_1d(N, (double*)&(s[0]), out, FFTW_ESTIMATE);
  
  fftw_execute(p);
  
  PF.resize(N/2 + 1);
  
  for(unsigned int i=0;i<PF.size();i++){
    PF[i] = (1.0/(N*sampling_hz)) * (out[i][0]*out[i][0] + out[i][1]*out[i][1]);
    if(i != 0 && i != PF.size() - 1)
      PF[i] *= 2.0;
  }
  
  hz_per_sample = sampling_hz/N;
  
  fftw_destroy_plan(p);
  fftw_free(out);
  
  return true;
}


// uses ||FFT(s)|| for band analysis
bool spectral_analysis(const std::vector<double>& s, const double sampling_hz,
		       double& delta, double& theta, double& alpha, 
		       double& beta,  double& gamma, double& mu)
{
  std::vector<double> PF;
  double hz_per_sample = 0.0;
  
  if(!power_spectral_analysis(s, sampling_hz, PF, hz_per_sample))
    return false;
  
  delta = 0.0;
  theta = 0.0;
  alpha = 0.0;
  beta  = 0.0;
  gamma = 0.0;
  mu    = 0.0;
  
  double hz = 0.0;
  
  for(unsigned int i=0;i<PF.size();i++){
    if(hz >= 0.0 && hz < 4.0)         delta += sqrt(PF[i]);
    else if(hz >= 4.0 && hz < 8.0)    theta += sqrt(PF[i]);
    else if(hz >= 8.0 && hz < 16.0)   alpha += sqrt(PF[i]);
    else if(hz >= 16.0 && hz < 32.0)   beta += sqrt(PF[i]);
    else if(hz >= 32.0 && hz < 100.0) gamma += sqrt(PF[i]);
    else if(hz >= 8.0 && hz < 12.0)      mu += sqrt(PF[i]);
    
    hz += hz_per_sample;
  }
  
  
  // normalizes power spectrum to be power/hz [does make sense?]

  delta /= (4.0 - 0.0);
  theta /= (8.0 - 4.0);
  alpha /= (16.0 - 8.0);
  beta  /= (32.0 - 16.0);
  gamma /= (100.0 - 32.0);
  mu    /= (12.0 - 8.0);
  
  return true;
}





