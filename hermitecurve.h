#ifndef __hermitecurve_h
#define __hermitecurve_h

#include <vector>
#include <dinrhiw/dinrhiw.h>

void createHermiteCurve(std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > >& samples,
			const unsigned int NPOINTS, const unsigned int DIMENSION,
			const unsigned int NSAMPLES);

#endif

