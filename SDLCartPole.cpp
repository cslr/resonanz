

#include "SDLCartPole.h"

#include <SDL.h>

namespace whiteice
{

  template <typename T>
  SDLCartPole<T>::SDLCartPole()
  {
    SDL_Init(SDL_INIT_VIDEO);

    window = NULL;
    renderer = NULL;

    W = 800;
    H = 600;

    SDL_CreateWindowAndRenderer(W, H, 0, &window, &renderer);
    
  }

  template <typename T>
  SDLCartPole<T>::~SDLCartPole()
  {
    if(renderer)
      SDL_DestroyRenderer(renderer);

    if(window)
      SDL_DestroyWindow(window);

    SDL_Quit();
  }

  template <typename T>
  bool SDLCartPole<T>::getState(whiteice::math::vertex<T>& state)
  {
    bool ok = CartPole<T>::getState(state);

    if(ok){
      auto theta = state[0];
      auto x     = state[2];

      SDL_Event event;
      while(SDL_PollEvent(&event)){
	if(event.type == SDL_QUIT){
	  this->running = false;
	  this->stop();
	}
      }

      // drawing
      {
	// black background
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	// SDL_RenderFillRect(renderer, NULL);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

	double y0 = H/2.0;
	double x0 = W/2.0 + x.c[0];

	double l = 100.0; // line length

	double y1 = y0 - l*cos(theta.c[0]);
	double x1 = x0 + l*sin(theta.c[0]);

	SDL_RenderDrawLine(renderer, (int)x0, (int)y0, (int)x1, (int)y1);
	SDL_RenderPresent(renderer);
      }
      
      printf("SDLCartPole<T>::getState() called: %f %f\n", theta.c[0], x.c[0]);
    }

    return ok;
  }

  template class SDLCartPole< math::blas_real<float> >;
  template class SDLCartPole< math::blas_real<double> >;
};
