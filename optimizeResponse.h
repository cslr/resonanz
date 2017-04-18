
#ifndef __resonanz_optimizeResponse_h
#define __resonanz_optimizeResponse_h

#include <dinrhiw.h>

namespace whiteice
{
  namespace resonanz
  {
    
    bool optimizeResponse(whiteice::nnetwork< whiteice::math::blas_real<double> >*& nn,
			  const whiteice::dataset< whiteice::math::blas_real<double> >& data);
    
    
  };
};




#endif
