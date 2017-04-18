
#include "hermitecurve.h"
#include <dinrhiw.h>
#include <vector>

using namespace whiteice;;


void createHermiteCurve(std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > >& samples,
			std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > points,
			double noise_stdev, // 0.02
			// const unsigned int NPOINTS, const unsigned int DIMENSION,
			const unsigned int NSAMPLES)
{
  // const unsigned int NPOINTS = 5;
  // const unsigned int DIMENSION = 2;
  // const unsigned int NSAMPLES = 10000; // 10.000 samples
	
  {
    // std::cout << "Generating d-dimensional curve dataset for learning.." << std::endl;

    if(points.size() <= 2) return; // need at least 2 points to interpolate between
    
    // pick N random points from DIMENSION dimensional space [-10,10]^D (initially N=5, DIMENSION=2)
    // calculate hermite curve interpolation between N points and generate data
    // add random noise N(0,1) to the spline to generate distribution
    // target is then to learn spline curve distribution
    
    {
      whiteice::math::hermite< math::vertex< math::blas_real<double> >, math::blas_real<double> > curve;
      whiteice::RNG< whiteice::math::blas_real<double> > rng;
      
      // std::vector< whiteice::math::vertex< math::blas_real<double> > > points;

      curve.calculate(points, (int)NSAMPLES);
      samples.clear();

      for(unsigned s=0;s<NSAMPLES;s++){
	auto& m = curve[s];
	auto  n = m;
	rng.normal(n);
	whiteice::math::blas_real<double> stdev = noise_stdev*rng.uniform();
	n = m + n*stdev;

	samples.push_back(n);
      }
    }

    {
      // normalizes mean and variance for each dimension
      whiteice::math::vertex< whiteice::math::blas_real<double> > m, v;
      const unsigned int DIMENSION = points[0].size();
      m.resize(DIMENSION);
      v.resize(DIMENSION);
      m.zero();
      v.zero();

      for(unsigned int s=0;s<NSAMPLES;s++){
	auto x = samples[s];
	m += x;
	
	for(unsigned int i=0;i<x.size();i++)
	  v[i] += x[i]*x[i];
      }
      
      m /= NSAMPLES;
      v /= NSAMPLES;

      for(unsigned int i=0;i<m.size();i++){
	v[i] -= m[i]*m[i];
	v[i] = sqrt(v[i]); // st.dev.
      }

      // normalizes mean to be zero and st.dev. / variance to be one
      for(unsigned int s=0;s<NSAMPLES;s++){
	auto x = samples[s];
	x -= m;
	
	for(unsigned int i=0;i<x.size();i++)
	  x[i] /= v[i];
	
	samples[s] = x;
#if 0
	auto& n    = samples[s];
	auto  z0   = n[2];
	z0         = 2.0;

	if(n[2] != 0.0){
	  n[0] = n[0] / (z0 + n[2]);
	  n[1] = n[1] / (z0 + n[2]);
	  n.resize(2);
	}
#endif
      }

      
    }

    // saveSamples("splinecurve.txt", samples);
  }
  
}
