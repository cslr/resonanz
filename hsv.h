/*
 * RGB to HSV colorspace and back converter
 * HSV colorspace should give qualitately better 
 *     results when using resonanz than using RGB colorspace
 * 
 */

#ifndef __resonanz_hsv_h
#define __resonanz_hsv_h

#include <dinrhiw.h>
#include <vector>
#include <string>

#include <SDL.h>


namespace whiteice {
  namespace resonanz {

    // opens and converts (RGB) picture to picsize*picsize*3 sized vector
    bool picToVector(const std::string& picture, const unsigned int picsize,
		     whiteice::math::vertex< whiteice::math::blas_real<double> >& v, bool hsv=true);

    // converts picture vector to allocated picsize*picsize SDL_Surface (RGB grayscale) for displaying and further use
    bool vectorToSurface(const whiteice::math::vertex< whiteice::math::blas_real<double> >& v,
			 const unsigned int picsize,
			 SDL_Surface*& surf, bool hsv=true);


    // values are within range 0-255
    
    void rgb2hsv(const unsigned int r, const unsigned int g, const unsigned int b,
		 unsigned int& h, unsigned int& s, unsigned int& v);

    void hsv2rgb(const unsigned int h, const unsigned int s, const unsigned int v,
		 unsigned int& r, unsigned int& g, unsigned int& b);


    
  };
};


#endif
