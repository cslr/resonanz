
#ifndef ts_measure_h
#define ts_measure_h

#include <vector>
#include <string>

#include <dinrhiw/dinrhiw.h>

#include "DataSource.h"

namespace whiteice{
  
  namespace resonanz{

        bool measureRandomPicturesAndTimeSeries(DataSource* dev,
						std::vector<std::string>& pictures,
						const unsigned int DISPLAYTIME,
						whiteice::dataset< whiteice::math::blas_real<double> >& timeseries);

  }
}

#endif
