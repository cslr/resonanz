

#ifndef __synth_stimulation_h
#define __synth_stimulation_h

#include <vector>
#include <string>

#include <dinrhiw.h>

#include "DataSource.h"

namespace whiteice {
  namespace resonanz {

    // stimulates CNS using randomly created decoder(features) = picture,
    // that maximizes nn(fearures, current_state) = next_state
    bool synthStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			  const unsigned picsize,
			  const unsigned int DISPLAYTIME,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			  DataSource* dev, 
			  whiteice::math::vertex< whiteice::math::blas_real<double> >& target,
			  whiteice::math::vertex< whiteice::math::blas_real<double> >& targetVariance);

    
    // stimulates CNS using given pictures that maximize nn(encoder(picture), current_state) = next_state
    bool pictureStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder,
			    const std::vector<std::string>& pictures,
			    const unsigned picsize,
			    const unsigned int DISPLAYTIME,
			    whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			    DataSource* dev, 
			    whiteice::math::vertex< whiteice::math::blas_real<double> >& target,
			    whiteice::math::vertex< whiteice::math::blas_real<double> >& targetVariance);

  }
}


#endif

