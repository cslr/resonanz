
#ifndef __pictureAutoencoder_h
#define __pictureAutoencoder_h 1

#include <dinrhiw.h>
#include <vector>
#include <string>

#include <SDL.h>
#include "hsv.h"

namespace whiteice
{
  namespace resonanz {
    

    // optimizes/learns autoencoder for processing pictures
    bool learnPictureAutoencoder(const std::string& picdir,
				 std::vector<std::string>& pictures,
				 unsigned int picsize,
				 whiteice::dataset< whiteice::math::blas_real<double> >& preprocess, 
				 whiteice::nnetwork< whiteice::math::blas_real<double> >*& encoder,
				 whiteice::nnetwork< whiteice::math::blas_real<double> >*& decoder);

    void generateRandomPictures(whiteice::dataset< whiteice::math::blas_real<double> >& preprocess,
				whiteice::nnetwork< whiteice::math::blas_real<double> >*& decoder);
    
  }
};


#endif
