
#ifndef __pictureAutoencoder_h
#define __pictureAutoencoder_h 1

#include <dinrhiw/dinrhiw.h>
#include <vector>
#include <string>

#include <SDL.h>


namespace whiteice
{
  namespace resonanz {
    
    // opens and converts (RGB) picture to picsize*picsize*3 sized vector
    bool picToVector(const std::string& picture, const unsigned int picsize,
		     whiteice::math::vertex< whiteice::math::blas_real<double> >& v, bool hsv=true);

    // converts picture vector to allocated picsize*picsize SDL_Surface (RGB grayscale) for displaying and further use
    bool vectorToSurface(const whiteice::math::vertex< whiteice::math::blas_real<double> >& v,
			 const unsigned int picsize,
			 SDL_Surface*& surf, bool hsv=true);

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
