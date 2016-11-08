
#include "measurements.h"
#include <SDL.h>

namespace whiteice
{
  namespace resonanz {

    bool measureResponses(DataSource* dev,
			  const unsigned int DISPLAYTIME, // msecs picture view time
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			  std::vector<std::string>& pictures,
			  const unsigned int picsize,
			  whiteice::dataset< whiteice::math::blas_real<double> >& data)
    {
      if(dev == NULL || DISPLAYTIME == 0 || encoder == NULL || decoder == NULL || picsize == 0)
	return false; // bad parameters
      
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
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
	
	
	SDL_Surface* win = SDL_GetWindowSurface(window);
      
	SDL_FillRect(win, NULL, 
		   SDL_MapRGB(win->format, 0, 0, 0));
	
	SDL_Surface* scaled = NULL;
	whiteice::math::vertex< whiteice::math::blas_real<double> > input;
	whiteice::math::vertex< whiteice::math::blas_real<double> > v;
	
	{ // synthesizes picture	  
	  
	  input.resize(decoder->input_size());
	  v.resize(decoder->output_size());
	  
	  // input values to decoder are 0/1 valued "sigmoidal" values
	  for(unsigned int i=0;i<input.size();i++){
	    input[i] = ((double)rand())/((double)RAND_MAX) > 0.5 ? 1.0 : 0.0;
	  }
	  
	  decoder->calculate(input, v);
	  
	  // std::cout << input << std::endl;
	  // std::cout << v << std::endl;
	  
	  const int picsize = (int)(sqrt(v.size()));
	  
	  if(vectorToSurface(v, picsize, scaled) == false)
	    continue;
	}
#if 0
	else{ // displays picture from disk (downscaled to picsize)

	  unsigned int r = rand() % pictures.size();
	  
	  input.resize(decoder->input_size());
	  v.resize(decoder->output_size());

	  if(picToVector(pictures[r], picsize, v)){

	    encoder->calculate(v, input); // generates feature from pic
	    
	    if(vectorToSurface(v, picsize, scaled) == false)
	      continue;
	  }
	}
#endif
	
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

	SDL_FreeSurface(scaled);

	SDL_UpdateWindowSurface(window);
	SDL_ShowWindow(window);
	SDL_FreeSurface(win);

	std::vector<float> before, after;

	if(dev->connectionOk() == false)
	  return false;

	if(dev->data(before) == false){
	  return false;
	}

	usleep(DISPLAYTIME*1000);

	if(dev->data(after) == false){
	  return false;
	}

	// stores measurement results to dataset
	{
	  if(data.getNumberOfClusters() != 3){
	    data.clear();

	    if(data.createCluster("input", input.size()) == false)
	      return false;

	    if(data.createCluster("before", before.size()) == false)
	      return false;

	    if(data.createCluster("after", after.size()) == false)
	      return false;
	  }

	  whiteice::math::vertex< whiteice::math::blas_real<double> > a;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > b;

	  b.resize(before.size());
	  a.resize(after.size());
	  
	  for(unsigned int i=0;i<b.size();i++)
	    b[i] = before[i];

	  for(unsigned int i=0;i<a.size();i++)
	    a[i] = after[i];

	  if(data.add(0, input) == false) return false;
	  if(data.add(1, b) == false) return false;
	  if(data.add(2, a)  == false) return false;
	}
	
      }
      
      SDL_DestroyWindow(window);

      return true;
    }
    
  };
};
