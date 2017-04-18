
#include "hsv.h"

#include <dinrhiw.h>
#include <SDL.h>
#include <SDL_image.h>

namespace whiteice
{
  namespace resonanz
  {


    // opens and converts (RGB) picture to picsize*picsize sized vector
    bool picToVector(const std::string& picture, const unsigned int picsize,
		     whiteice::math::vertex< whiteice::math::blas_real<double> >& vec, bool hsv)
    {
      if(picsize <= 0) return false;

      // tries to open picture
      SDL_Surface* pic = IMG_Load(picture.c_str());

      if(pic == NULL) return false;

      // converts to picsize x picsize format
      SDL_Surface* scaled = NULL;
      
      scaled = SDL_CreateRGBSurface(0, picsize, picsize, 32,
				    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

      if(scaled == NULL){
	SDL_FreeSurface(pic);
	return false;
      }

      SDL_Rect srcrect;

      if(pic->w < pic->h){
	srcrect.w = pic->w;
	srcrect.x = 0;
	srcrect.h = pic->w;
	srcrect.y = (pic->h - pic->w)/2;
      }
      else{
	srcrect.h = pic->h;
	srcrect.y = 0;
	srcrect.w = pic->h;
	srcrect.x = (pic->w - pic->h)/2;
      }

      if(SDL_BlitScaled(pic, &srcrect, scaled, NULL) != 0){
	SDL_FreeSurface(pic);
	SDL_FreeSurface(scaled);
	return false;
      }

      SDL_FreeSurface(pic);

#if 1
      // transforms picture to a vector (HSV)
      vec.resize(3*picsize*picsize);

      unsigned int index=0;

      for(int j=0;j<scaled->h;j++){
	for(int i=0;i<scaled->w;i++){
	  if(hsv){
	    unsigned int pixel = ((unsigned int*)(((char*)scaled->pixels) + j*scaled->pitch))[i];
	    unsigned int r = (0x00FF0000 & pixel) >> 16;
	    unsigned int g = (0x0000FF00 & pixel) >>  8;
	    unsigned int b = (0x000000FF & pixel);
	    
	    unsigned int h, s, v;
	    rgb2hsv(r,g,b,h,s,v);
	    
	    vec[index] = (double)h/255.0; index++;
	    vec[index] = (double)s/255.0; index++;
	    vec[index] = (double)v/255.0; index++;
	  }
	  else{
	    unsigned int pixel = ((unsigned int*)(((char*)scaled->pixels) + j*scaled->pitch))[i];
	    unsigned int r = (0x00FF0000 & pixel) >> 16;
	    unsigned int g = (0x0000FF00 & pixel) >>  8;
	    unsigned int b = (0x000000FF & pixel);

	    vec[index] = (double)r/255.0; index++;
	    vec[index] = (double)g/255.0; index++;
	    vec[index] = (double)b/255.0; index++;
	  }
	}
      }
#endif
      
      SDL_FreeSurface(scaled);
      
      
      return true;
    }

    // converts picture vector to allocated picsize*picsize SDL_Surface (RGB) for displaying and further use
    bool vectorToSurface(const whiteice::math::vertex< whiteice::math::blas_real<double> >& vec,
			 const unsigned int picsize,
			 SDL_Surface*& surf, bool hsv)
    {

#if 1
      if(picsize <= 0) return false;
      if(vec.size() != 3*picsize*picsize) return false;
      
      surf = NULL;
      
      surf = SDL_CreateRGBSurface(0, picsize, picsize, 32,
				  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
      
      if(surf == NULL){
	return false;
      }

      unsigned int index = 0;

      for(int j=0;j<surf->h;j++){
	for(int i=0;i<surf->w;i++){
	  if(hsv){
	    unsigned int r = 0, g = 0, b = 0;
	    unsigned int h = 0, s = 0, v = 0;
	    int sh = 0, ss = 0, sv = 0;
	    
	    whiteice::math::convert(sh, 255.0*vec[index]); index++;
	    whiteice::math::convert(ss, 255.0*vec[index]); index++;
	    whiteice::math::convert(sv, 255.0*vec[index]); index++;
	    
	    if(sh<0) sh = 0; if(sh>255) sh = 255;
	    if(ss<0) ss = 0; if(ss>255) ss = 255;
	    if(sv<0) sv = 0; if(sv>255) sv = 255;
	    
	    h = sh;
	    s = ss;
	    v = sv;
	    
	    hsv2rgb(h,s,v,r,g,b);
	    
	    const unsigned int pixel = (((unsigned int)r)<<16) + (((unsigned int)g)<<8) + (((unsigned int)b)<<0) + 0xFF000000;
	    
	    ((unsigned int*)(((char*)surf->pixels) + j*surf->pitch))[i] = pixel;
	  }
	  else{
	    unsigned int r = 0, g = 0, b = 0;
	    
	    whiteice::math::convert(r, 255.0*vec[index]); index++;
	    whiteice::math::convert(g, 255.0*vec[index]); index++;
	    whiteice::math::convert(b, 255.0*vec[index]); index++;

	    const unsigned int pixel = (((unsigned int)r)<<16) + (((unsigned int)g)<<8) + (((unsigned int)b)<<0) + 0xFF000000;
	    
	    ((unsigned int*)(((char*)surf->pixels) + j*surf->pitch))[i] = pixel;
	  }
	}
      }
      
#endif
      
      return true;
    }
    


    
    // values are within range 0-255
    
    void rgb2hsv(const unsigned int r, const unsigned int g, const unsigned int b,
		 unsigned int& h, unsigned int& s, unsigned int& v)
    {
      unsigned char rgbMin, rgbMax;

      rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
      rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

      v = rgbMax;
      if (v == 0)
      {
        h = 0;
        s = 0;
	return;
      }

      s = 255 * long(rgbMax - rgbMin) / v;
      if (s == 0)
      {
        h = 0;
        return;
      }

      if (rgbMax == r)
        h = 0 + 43 * (g - b) / (rgbMax - rgbMin);
      else if (rgbMax == g)
        h = 85 + 43 * (b - r) / (rgbMax - rgbMin);
      else
        h = 171 + 43 * (r - g) / (rgbMax - rgbMin);
      
      return;
    }
    

    void hsv2rgb(const unsigned int h, const unsigned int s, const unsigned int v,
		 unsigned int& r, unsigned int& g, unsigned int& b)
    {
      unsigned char region, remainder, p, q, t;

      if (s == 0)
      {
        r = v;
        g = v;
        b = v;
        return;
      }
      
      region = h / 43;
      remainder = (h - (region * 43)) * 6; 

      p = (v * (255 - s)) >> 8;
      q = (v * (255 - ((s * remainder) >> 8))) >> 8;
      t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
      
      switch (region)
      {
      case 0:
	r = v; g = t; b = p;
	break;
      case 1:
	r = q; g = v; b = p;
	break;
      case 2:
	r = p; g = v; b = t;
	break;
      case 3:
	r = p; g = q; b = v;
	break;
      case 4:
	r = t; g = p; b = v;
	break;
      default:
	r = v; g = p; b = q;
	break;
      }
      
      return;
    }
    
    
  }
}
