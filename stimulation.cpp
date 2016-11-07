
#include "stimulation.h"
#include <assert.h>
#include <SDL.h>

namespace whiteice {
  namespace resonanz {

        bool synthStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			      const unsigned picsize,
			      const unsigned int DISPLAYTIME,
			      whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			      whiteice::math::vertex< whiteice::math::blas_real<double> >& target)
	{
	  // create a window and create N (within displaytime/2 time) random attempts
	  // to get smallest error towards target
	  
	  
	  return false;
	}
    
    
  }
}
