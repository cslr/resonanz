/*
 * Cart Pole problem SDL visualization
 */

#ifndef __whiteice__sdl_cartpole_h
#define __whiteice__sdl_cartpole_h

#include <dinrhiw/dinrhiw.h>

#include <SDL.h>

namespace whiteice
{
  template <typename T>
    class SDLCartPole : public CartPole<T>
    {
    public:
      SDLCartPole();
      ~SDLCartPole();

    protected:

      // overrides getState
      // each call to getState() updates current state (graphics)
      virtual bool getState(whiteice::math::vertex<T>& state);

      int W, H;

      SDL_Window* window;
      SDL_Renderer* renderer;
      
    };
  
  extern template class SDLCartPole< math::blas_real<float> >;
  extern template class SDLCartPole< math::blas_real<double> >;
};



#endif

