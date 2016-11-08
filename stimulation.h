

#ifndef __synth_stimulation_h
#define __synth_stimulation_h

#include <dinrhiw/dinrhiw.h>
#include "DataSource.h"

namespace whiteice {
  namespace resonanz {
    
    bool synthStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			  const unsigned picsize,
			  const unsigned int DISPLAYTIME,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			  DataSource* dev, 
			  whiteice::math::vertex< whiteice::math::blas_real<double> >& target,
			  whiteice::math::vertex< whiteice::math::blas_real<double> >& targetVariance);

  }
}


#endif

