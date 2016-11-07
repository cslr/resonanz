

#ifndef __synth_stimulation_h
#define __synth_stimulation_h

#include <dinrhiw/dinrhiw.h>

namespace whiteice {
  namespace resonanz {
    
    bool synthStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			  const unsigned picsize,
			  const unsigned int DISPLAYTIME,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			  whiteice::math::vertex< whiteice::math::blas_real<double> >& target);

  }
}


#endif

