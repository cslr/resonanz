
#include "pictureAutoencoder.h"

#include <SDL.h>
#include <SDL_image.h>


namespace whiteice
{
  namespace resonanz {

    
    // opens and converts (RGB) picture to 3*picsize*picsize sized vector
    bool picToVector(const std::string& picture, const unsigned int picsize,
		     whiteice::math::vertex< whiteice::math::blas_real<double> >& v)
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

      // FIXME
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

      // transforms picture to a vector
      v.resize(3*picsize*picsize);

      unsigned int index=0;

      for(int j=0;j<scaled->h;j++){
	for(int i=0;i<scaled->w;i++){
	  unsigned int pixel = ((unsigned int*)(((char*)scaled->pixels) + j*scaled->pitch))[i];
	  int r = (0x00FF0000 & pixel) >> 16;
	  int g = (0x0000FF00 & pixel) >>  8;
	  int b = (0x000000FF & pixel);

	  v[index] = (double)r; index++;
	  v[index] = (double)g; index++;
	  v[index] = (double)b; index++;
	}
      }

      SDL_FreeSurface(scaled);
      
      return true;
    }
    

    // converts picture vector to allocated picsize*picsize SDL_Surface (RGB) for displaying and further use
    bool vectorToSurface(const whiteice::math::vertex< whiteice::math::blas_real<double> >& v,
			 const unsigned int picsize,
			 SDL_Surface*& surf)
    {
      if(picsize <= 0) return false;
      if(v.size() != 3*picsize*picsize) return false;
      
      surf = NULL;
      
      surf = SDL_CreateRGBSurface(0, picsize, picsize, 32,
				  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
      
      if(surf == NULL){
	return false;
      }

      unsigned int index = 0;

      for(int j=0;j<surf->h;j++){
	for(int i=0;i<surf->w;i++){
	  int r = 0, g = 0, b = 0;

	  whiteice::math::convert(r, v[index]); index++;
	  whiteice::math::convert(g, v[index]); index++;
	  whiteice::math::convert(b, v[index]); index++;
	  
	  if(r<0) r = 0; if(r>255) r = 255;
	  if(g<0) g = 0; if(g>255) g = 255;
	  if(b<0) b = 0; if(b>255) b = 255;
	  
	  const unsigned int pixel = (((unsigned int)r)<<16) + (((unsigned int)g)<<8) + (((unsigned int)b)<<0) + 0xFF000000;
	  
	  ((unsigned int*)(((char*)surf->pixels) + j*surf->pitch))[i] = pixel;
	}
      }
      
      return true;
    }

    
    // optimizes/learns autoencoder for processing pictures
    bool learnPictureAutoencoder(const std::string& picdir,
				 std::vector<std::string>& pictures,
				 unsigned int picsize,
				 whiteice::nnetwork< whiteice::math::blas_real<double> >& encoder,
				 whiteice::nnetwork< whiteice::math::blas_real<double> >& decoder)
    {
      // INITIAL TEST TO TEST PICTURE LOADING WORKS OK

      // open window (SDL)

      SDL_Window* window = NULL;
      
      int W = 640;
      int H = 480;
      
      
      SDL_DisplayMode mode;
      
      if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
	W = (3*mode.w)/4;
	H = (3*mode.h)/4;
      }

      window = SDL_CreateWindow("Renaissance",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				W, H,
				SDL_WINDOW_ALWAYS_ON_TOP |
				SDL_WINDOW_INPUT_FOCUS);
      SDL_RaiseWindow(window);
      SDL_UpdateWindowSurface(window);
      SDL_RaiseWindow(window);

      SDL_Event event;
      bool exit = false;

      while(!exit){
	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN)
	    exit = true;
	}
	
	
	SDL_Surface* win = SDL_GetWindowSurface(window);
      
	SDL_FillRect(win, NULL, 
		   SDL_MapRGB(win->format, 0, 0, 0));
	
	SDL_Surface* scaled = NULL;

	whiteice::math::vertex< whiteice::math::blas_real<double> > v;
	
	if(picToVector(pictures[rand() % pictures.size()], picsize, v) == false) continue;

	if(vectorToSurface(v, picsize, scaled) == false) continue;
	

	SDL_Rect imageRect;
	
	if(win->w < win->h){
	  imageRect.w = win->w;
	  imageRect.h = win->w;
	  imageRect.x = 0;
	  imageRect.y = (H - win->w)/2;
	}
	else{
	  imageRect.w = win->h;
	  imageRect.h = win->h;
	  imageRect.x = (W - win->h)/2;
	  imageRect.y = 0;
	  
	}

	// SDL_BlitSurface(scaled, NULL, win, &imageRect);
	SDL_BlitScaled(scaled, NULL, win, &imageRect);

	SDL_UpdateWindowSurface(window);
	SDL_ShowWindow(window);
	SDL_FreeSurface(win);
		
      }
		 

      
      return false;
    }
    
  }
}
