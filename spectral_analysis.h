/*
 * Calculates spectral analysis of real valued 1d signal data (EEG)
 * This can be then used to calculate power specturum of the data
 * and to estimate power in EEG bands: 
 *
 * delta 0-4    Hz (slow wave sleep)
 * theta 4-7    Hz (drowsiness / idling)
 * alpha 8-15   Hz (relaxed)
 * beta  16-31  Hz (alert, active)
 * gamma 32-100 Hz (cogntively active)
 * mu    8-12   Hz (sensimotor cortex)
 *
 * Power is normalized as P = sum(|FFT[lo_hz .. hi_hz}|) / (hi_hz - lo_hz)
 */ 

#ifndef __spectral_analysis_h
#define __spectral_analysis_h

#include <vector>

/*
 * returns power in different frequencies of source signal s with sampling frequency sampling_hz
 */
bool spectral_analysis(const std::vector<double>& s, const double sampling_hz,
		       double& delta, double& theta, double& alpha, 
		       double& beta,  double& gamma, double& mu);

/*
 * returns power spectrum of real valued source signal s and returns number of hz per sample in PF
 * power spectrum is |FFT(s)|^2
 */
bool power_spectral_analysis(const std::vector<double>& s, const double sampling_hz,
			     std::vector<double>& PF, double& hz_per_sample);


#endif

