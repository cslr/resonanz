/*
 * RGB to HSV colorspace and back converter
 * HSV colorspace should give qualitately better 
 *     results when using resonanz than using RGB colorspace
 * 
 */

#ifndef __resonanz_hsv_h
#define __resonanz_hsv_h

namespace whiteice {
  namespace resonanz {

    // values are within range 0-255
    
    void rgb2hsv(const unsigned int r, const unsigned int g, const unsigned int b,
		 unsigned int& h, unsigned int& s, unsigned int& v);

    void hsv2rgb(const unsigned int h, const unsigned int s, const unsigned int v,
		 unsigned int& r, unsigned int& g, unsigned int& b);


    
  };
};


#endif
