
#include "stimulation.h"
#include <assert.h>
#include <SDL.h>

#include "pictureAutoencoder.h"

#include <chrono>
using namespace std::chrono;


namespace whiteice {
  namespace resonanz {
    
        bool synthStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder,
			      const unsigned picsize,
			      const unsigned int DISPLAYTIME,
			      whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			      DataSource* dev, 
			      whiteice::math::vertex< whiteice::math::blas_real<double> >& target,
			      whiteice::math::vertex< whiteice::math::blas_real<double> >& targetVariance)
	{
	  // create a window and create N (within displaytime/2 time)
	  // random attempts to get smallest error towards target

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
	  
	  whiteice::math::vertex< whiteice::math::blas_real<double> > v;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > input;
	  input.resize(decoder->input_size());
	  v.resize(decoder->output_size());
	  input.zero();
	  
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

	    auto best = input;
	    double best_error = INFINITY;
	    
	    long long startTime = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	    long long endTime = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		    
	    
	    // generates random solutions during display time and selects the best one found
	    while((endTime - startTime) < DISPLAYTIME){
	      
	      for(unsigned int i=0;i<input.size();i++){
		double value = ((double)rand()/((double)RAND_MAX));
		if(value < 0.5) value = 0.0;
		else value = 1.0;
		
		input[i] = value;
	      }

	      whiteice::math::vertex< whiteice::math::blas_real<double> > in(response->input_size());

	      std::vector<float> measurement; // current state
	      dev->data(measurement);
	      
	      whiteice::math::vertex< whiteice::math::blas_real<double> > current;
	      whiteice::math::vertex< whiteice::math::blas_real<double> > predicted;
	      current.resize(measurement.size());
	      predicted.resize(measurement.size());
	      
	      for(unsigned int i=0;i<current.size();i++)
		current[i] = measurement[i];

	      in.write_subvertex(input, 0);
	      in.write_subvertex(current, input.size());

	      // calculates expected response nn(picture_feature, current_state) = next state
	      
	      response->calculate(in, predicted);
	      
	      // calculates distance to target
	      double error = 0.0;
	      for(unsigned int i=0;i<target.size();i++){
		auto d = (target[i] - predicted[i])*(target[i] - predicted[i])/targetVariance[i];
		error += d.c[0];
	      }

	      if(error <= best_error){
		// found a better solution
		error = best_error;
		best = input;
	      }

	      endTime = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	    }

	    
	    
	    decoder->calculate(best, v);
	    
	    // std::cout << input << std::endl;
	    // std::cout << v << std::endl;
	    
	    const int picsize = (int)(sqrt(v.size()));
	    
	    if(vectorToSurface(v, picsize, scaled) == false)
	      continue;
	    
	    
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
	    
	  }
	  
	  
	  SDL_DestroyWindow(window);
	  
	  
	  return true;
	}


    // stimulates CNS using given pictures that maximize nn(encoder(picture), current_state) = next_state
    bool pictureStimulation(whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder,
			    const std::vector<std::string>& pictures,
			    const unsigned picsize,
			    const unsigned int DISPLAYTIME,
			    whiteice::nnetwork< whiteice::math::blas_real<double> >* response,
			    DataSource* dev, 
			    whiteice::math::vertex< whiteice::math::blas_real<double> >& target,
			    whiteice::math::vertex< whiteice::math::blas_real<double> >& targetVariance)
    {
      // create a window and create N (within displaytime/2 time)
      // random attempts to get smallest error towards target
      // open window (SDL)

      if(pictures.size() <= 0)
	return false;
      
      
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
      
      whiteice::math::vertex< whiteice::math::blas_real<double> > v;
      whiteice::math::vertex< whiteice::math::blas_real<double> > input;
      input.resize(encoder->output_size());
      v.resize(encoder->input_size());
      input.zero();
      
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

	whiteice::math::vertex< whiteice::math::blas_real<double> > bestv;
	double best_error = INFINITY;

	if(picToVector(pictures[0], picsize, bestv) == false){
	  return false;
	}
	
	long long startTime = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	long long endTime = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	
	
	// generates random solutions during display time and selects the best one found
	while((endTime - startTime) < DISPLAYTIME){
	  const unsigned int index = rand() % pictures.size();
	  whiteice::math::vertex< whiteice::math::blas_real<double> > input;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > pic;
	  
	  if(picToVector(pictures[index], picsize, pic) == false)
	    continue;

	  if(encoder->calculate(pic, input) == false) // calculates feature vector from picture
	    continue;
	  
	  whiteice::math::vertex< whiteice::math::blas_real<double> > in(response->input_size());
	  
	  std::vector<float> measurement; // current state
	  dev->data(measurement);
	  
	  whiteice::math::vertex< whiteice::math::blas_real<double> > current;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > predicted;
	  current.resize(measurement.size());
	  predicted.resize(measurement.size());
	  
	  for(unsigned int i=0;i<current.size();i++)
	    current[i] = measurement[i];

	  in.write_subvertex(input, 0);
	  in.write_subvertex(current, input.size());
	  
	  // calculates expected response nn(picture_feature, current_state) = next state
	  
	  response->calculate(in, predicted);
	  
	  // calculates distance to target
	  double error = 0.0;
	  for(unsigned int i=0;i<target.size();i++){
	    auto d = (target[i] - predicted[i])*(target[i] - predicted[i])/targetVariance[i];
	    error += d.c[0];
	  }
	  
	  if(error <= best_error){
	    // found a better solution
	    error = best_error;
	    bestv = pic;
	  }
	  
	  endTime = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}
	
	v = bestv;
	
	// std::cout << input << std::endl;
	// std::cout << v << std::endl;
	
	const int picsize = (int)(sqrt(v.size()));
	
	if(vectorToSurface(v, picsize, scaled) == false)
	  continue;
	
	    
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
	
      }
      
      
      SDL_DestroyWindow(window);
      
      
      return true;
    }
    
    
  }
}
