
#ifndef __neuralnetwork_measurements_h
#define __neuralnetwork_measurements_h

#include "DataSource.h"
#include <dinrhiw/dinrhiw.h>
#include <vector>
#include <string>
#include "pictureAutoencoder.h"


namespace whiteice
{
  namespace resonanz {


    bool measureResponses(DataSource* dev,
			  const unsigned int DISPLAYTIME, // msecs picture view time
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			  std::vector<std::string>& pictures,
			  const unsigned int picsize,
			  whiteice::dataset< whiteice::math::blas_real<double> >& data);
    
  };
};

#endif
