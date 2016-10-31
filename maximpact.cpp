/*
 * MaxImpact - create maximum reponse stimulus pictures (see PLAN.txt) 
 * 
 */

#include <stdio.h>
#include <sys/stat.h>
#include <vector>
#include <string>

#include <dinrhiw.h>
#include <ncurses.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL2_rotozoom.h>


void print_show_usage(){
  printf("Usage: maximpact <picture>\n");
}


bool parse_parameters(int argc, char** argv, std::string& filename)
{
  if(argc != 2) return false;
  filename = argv[1];
  return true;
}


double* createRandomSurface(int w, int h, double stdev);

// calculates real value of gabor filter
bool calculateGaborFilter(std::vector<double>& p, 
			  std::vector<double>& filter, 
			  unsigned int N, double L);

// calculates convolution of surface per each color component
bool convolveSurface(SDL_Surface* original, 
		     std::vector<double>& filter, 
		     unsigned int N, SDL_Surface* scaled); 



int main(int argc, char** argv)
{
  printf("MaxImpact v0.01\n");
  
  srand(time(0));
  
  std::string picture;

  if(!parse_parameters(argc, argv, picture)){
    print_show_usage();
    fprintf(stderr, "ERROR: Incorrect parameters.\n");
    return -1;
  }
  else{
    struct stat buf;

    if(stat(picture.c_str(), &buf) != 0){
      print_show_usage();
      fprintf(stderr, "ERROR: Incorrect parameters.\n");
      return -1;    
    }
  }

  
  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  
  SDL_Surface* pic = IMG_Load(picture.c_str());
  
  if(pic == NULL){
    printf("Usage: maximpact <picture>\n");
    fprintf(stderr, "ERROR: Bad picture file.\n");
    
    IMG_Quit();
    SDL_Quit();
    
    return -1;    
  }
  
  SDL_Window* window = NULL;
  
  int W = 640;
  int H = 480;


  SDL_DisplayMode mode;
  
  if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
    W = (3*mode.w)/4;
    H = (3*mode.h)/4;
  }

  
  
  window = SDL_CreateWindow("MaxImpact",
			    SDL_WINDOWPOS_CENTERED,
			    SDL_WINDOWPOS_CENTERED,
			    W, H, 0);
  if(window == NULL){
    fprintf(stderr, "ERROR: Cannot open window: %s.\n", SDL_GetError());
    SDL_FreeSurface(pic);
    IMG_Quit();
    SDL_Quit();
    
    return -1;        
  }

  
  SDL_Event event;
  bool exit = false;

  // converts picture to scaled version of itself that fits to the window
  SDL_Surface* scaled = NULL;

  if((pic->w) > (pic->h)){
    double wscale = ((double)W)/((double)pic->w);
    
    scaled = SDL_CreateRGBSurface(0, (int)(pic->w*wscale), (int)(pic->h*wscale), 32,
				  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    if(SDL_BlitScaled(pic, NULL, scaled, NULL) != 0)
      return -1;
  }
  else{
    double hscale = ((double)H)/((double)pic->h);
    
    scaled = SDL_CreateRGBSurface(0, (int)(pic->w*hscale), (int)(pic->h*hscale), 32,
				  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    
    if(SDL_BlitScaled(pic, NULL, scaled, NULL) != 0)
      return -1;
  }
  
  SDL_FreeSurface(pic);

  // std::vector<double> p; // gabor filter parameters
  // p.resize(5);
  
  // std::vector<double> filter; // 33x33 filter mask
  // filter.resize(33*33);
  
  
  while(!exit){
    // creates random delta D for scaled picture and measures how much responses change in dt time.
    // this will then used to calculate E[ gradient ] along which next step is searched for
    
    double* delta = createRandomSurface(scaled->w, scaled->h, 100.0); // stdev is range of changes
    
    // calculates convolution of surface per each color component
    // calculateGaborFilter(p, filter, 16, 0.1);
    // convolveSurface(original, filter, 16, scaled); 
    {
      // SDL graphics code (TODO implement me)
      
      SDL_Surface* surf = SDL_GetWindowSurface(window);
      
      SDL_FillRect(surf, NULL, 
		   SDL_MapRGB(surf->format, 0, 0, 0));

      SDL_Surface* view = SDL_CreateRGBSurface(0, scaled->w, scaled->h, 32,
					       0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

      SDL_Rect imageRect;
      imageRect.w = scaled->w;
      imageRect.h = scaled->h;
      imageRect.x = (W - scaled->w)/2;
      imageRect.y = (H - scaled->h)/2;

      for(int j=0;j<scaled->h;j++){
	for(int i=0;i<scaled->w;i++){
	  unsigned int pixel = ((unsigned int*)(((char*)scaled->pixels) + j*scaled->pitch))[i];
	  int r = (0x00FF0000 & pixel) >> 16;
	  int g = (0x0000FF00 & pixel) >>  8;
	  int b = (0x000000FF & pixel);

	  r += delta[3*(j*scaled->w + i) + 0];
	  g += delta[3*(j*scaled->w + i) + 1];
	  b += delta[3*(j*scaled->w + i) + 2];

	  if(r<0) r = 0; if(r>255) r = 255;
	  if(g<0) g = 0; if(g>255) g = 255;
	  if(b<0) b = 0; if(b>255) b = 255;
	  
	  pixel = (((unsigned int)r)<<16) + (((unsigned int)g)<<8) + (((unsigned int)b)<<0) + 0xFF000000;

	  ((unsigned int*)(((char*)view->pixels) + j*view->pitch))[i] = pixel;
	}
      }

	    
      SDL_BlitSurface(view, NULL, surf, &imageRect);
      
      
      SDL_FreeSurface(surf);
      SDL_UpdateWindowSurface(window);
      SDL_ShowWindow(window);
      
      while(SDL_PollEvent(&event)){
	if(event.type == SDL_KEYDOWN)
	  exit = true;
      }
    }
    
    delete[] delta;
  }
  

  SDL_FreeSurface(scaled);
  if(window) SDL_DestroyWindow(window);
  
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}




double* createRandomSurface(int w, int h, double stdev)
{
  double* delta = new double[w*h*3];

  whiteice::RNG<double> rng;

  for(int j=0;j<h;j++){
    for(int i=0;i<w;i++){
      delta[3*(j*w + i) + 0] = rng.uniform()*stdev;
      delta[3*(j*w + i) + 1] = rng.uniform()*stdev;
      delta[3*(j*w + i) + 2] = rng.uniform()*stdev;
    }
  }
  
  return delta;
}



/*
 * calculates reao value of gabor filter for the given filter mask
 * p[0] = theta  (rotation in radians)
 * p[1] = lambda (sinusoid "wavelength")
 * p[2] = sigma  (gaussian "wavelength")
 * p[3] = gamma  (ellipticity) [1.0 = circle]
 * p[4] = delta  (phase offset of sinusoid)
 *
 * calculates real valuec of filter: 
 * gabor filter of size 2*N+1 and of length L (N = L units long)
 */
bool calculateGaborFilter(std::vector<double>& p, 
			  std::vector<double>& filter, 
			  unsigned int N, double L)
{
  if(p.size() != 5) return false;
  if(N <= 2) return false;
  if(L <= 0.0) return false;
  
  const double theta  = p[0];
  const double lambda = p[1];
  const double sigma  = p[2];
  const double gamma  = p[3];
  const double delta  = p[4];
  
  filter.resize((2*N + 1)*(2*N + 1));
  
  double sum = 0.0;
  
  for(int y=-N;y<N;y++){
    for(int x=-N;x<N;x++){
      const double ry = (y/L);
      const double rx = (x/L);
      
      const double rxp = + rx*cos(theta) + ry*sin(theta);
      const double ryp = - rx*sin(theta) + ry*cos(theta);
      
      const unsigned int i = (unsigned int)(x+N);
      const unsigned int j = (unsigned int)(y+N);
      
      double rr = exp(-(rxp + gamma*ryp)/(2.0*sigma*sigma)) * cos(2*M_PI*rxp/lambda + delta);
      double ii = exp(-(rxp + gamma*ryp)/(2.0*sigma*sigma)) * sin(2*M_PI*rxp/lambda + delta);
      
      filter[i + j*(2*N+1)] = rr; // sqrt(rr*rr + ii*ii);
      
      sum += rr;
    }
  }
  
  for(int y=-N;y<N;y++){
    for(int x=-N;x<N;x++){
      const unsigned int i = (unsigned int)(x+N);
      const unsigned int j = (unsigned int)(y+N);
      
      filter[i + j*(2*N+1)] /= sum;
    }
  }
  
  // so total sum of weights is equal to 1
  
  
  return true;
}


// calculates convolution of surface per each color component
bool convolveSurface(SDL_Surface* original, 
		     std::vector<double>& filter, 
		     unsigned int N, 
		     SDL_Surface* scaled)
{
  if(original->format->format != SDL_PIXELFORMAT_ARGB8888)
    return false;
  
  Uint32* in  = (Uint32*)(original->pixels);
  Uint32* out = (Uint32*)(scaled->pixels);
  
  for(int j=0;j<(original->h);j++){
    for(int i=0;i<(original->w);i++){
      double result_r = 0.0;
      double result_g = 0.0;
      double result_b = 0.0;
      double points   = 0.0;

      for(int y=-N;y<N;y++){
	for(int x=-N;x<N;x++){
	  if((j+y >= 0) && (j+y < original->h) && (i+x >= 0) && (i+x < original->w)){
	    result_r += filter[(2*N+1-(x+N)) + (2*N+1-(y+N))*(2*N+1)] * ((in[(x+i) + (y+j)*(original->w)]>>16) & 0xFF);
	    result_g += filter[(2*N+1-(x+N)) + (2*N+1-(y+N))*(2*N+1)] * ((in[(x+i) + (y+j)*(original->w)]>> 8) & 0xFF);
	    result_b += filter[(2*N+1-(x+N)) + (2*N+1-(y+N))*(2*N+1)] * ((in[(x+i) + (y+j)*(original->w)]>> 0) & 0xFF);
	    points++;
	  }
	}
      }

      points /= ((2*N+1)*(2*N+1));
      
      result_r /= points;
      result_g /= points;
      result_b /= points;
      
      out[i + j*(original->w)] = 
	(((Uint32)result_r) & 0xFF) << 16 + 
	(((Uint32)result_g) & 0xFF) << 8 + 
	(((Uint32)result_b) & 0xFF) << 0;
	
    }
  }
  
  return true;
}


