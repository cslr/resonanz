
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

	bool predictHiddenState(DataSource* dev,
				const whiteice::HMM& hmm, 
				std::vector<std::string>& pictures,
				const unsigned int DISPLAYTIME,
				whiteice::dataset< whiteice::math::blas_real<double> >& timeseries);

	bool measureRandomPictures(DataSource* dev,
				   const whiteice::HMM& hmm, 
				   std::vector<std::string>& pictures,
				   const unsigned int DISPLAYTIME,
				   whiteice::dataset< whiteice::math::blas_real<double> >& timeseries, 
				   std::vector< whiteice::dataset< whiteice::math::blas_real<double> > >& data);

	unsigned int discretize(const std::vector<double>& data,
				const std::vector<double>& m,
				const std::vector<double>& s);
	
  }
}

#endif
