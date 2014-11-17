/*
 * MaxImpact - create maximum reponse stimulus pictures (see PLAN.txt) 
 * [use GA/genetic algorithm optimization from dinrhiw + 
 *  measure brainwave responses]
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
#include <aalib.h>

// NOTE: conversion of image to ASCII does not really work in console


void print_show_usage(){
  printf("Usage: maximpact <picture> [--ascii] [AA-lib options]\n");
}


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
  struct stat buf;
  
  // to allow development/testing in text only environment
  bool useAscii = false; 
  
  if(argc < 2){
    print_show_usage();
    fprintf(stderr, "ERROR: Incorrect number of parameters.\n");
    return -1;
  }
  else if(stat(argv[1], &buf) != 0){
    print_show_usage();
    fprintf(stderr, "ERROR: Incorrect parameters.\n");
    return -1;    
  }
  else{
    if(buf.st_size > 0){
      picture = argv[1];
    }
    else{
      print_show_usage();
      fprintf(stderr, "ERROR: Bad picture file.\n");
      return -1;
    }
  }
  
  for(unsigned int i=2;i<3;i++){
    if(strcmp(argv[i], "--ascii") == 0){
      useAscii = true;
    }
    else{
      printf("%s\n", argv[i]);
      print_show_usage();
      fprintf(stderr, "ERROR: bad command line parameter.\n");
      return -1;
    }
  }

  if(useAscii){
    int nargc = argc-2;
    char** nargv = &(argv[2]);
    if(!aa_parseoptions(NULL, NULL, &nargc, nargv)){
      print_show_usage();
      fprintf(stderr, "ERROR: BAD AAlib parameters.\n");
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
  aa_context* ascii = NULL;
  
  unsigned int W = 160;
  unsigned int H = 120;
  
  if(!useAscii){
    window = SDL_CreateWindow("MaxImpact",
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      W, H, 0);
  }
  
  if(window == NULL){
    ascii = aa_autoinit(&aa_defparams);
    
    if(ascii){
      W = aa_scrwidth(ascii);
      H = aa_scrheight(ascii);
      
      printf("AAlib output: %dx%d\n", W, H);
      fflush(stdout);
      
      aa_autoinitkbd(ascii, 0);
      initscr();
      raw();
      noecho();
      curs_set(0);
    }
  }
  

  if(window == NULL && ascii == NULL){
    if(window == NULL) 
      fprintf(stderr, "ERROR: Cannot open window: %s.\n", SDL_GetError());
    SDL_FreeSurface(pic);
    IMG_Quit();
    SDL_Quit();
    
    return -1;        
  }
  
  SDL_Event event;
  bool exit = false;
  
  SDL_Surface* original = NULL;
  SDL_PixelFormat fmt;
  fmt.format = SDL_PIXELFORMAT_ARGB8888;
  fmt.BitsPerPixel  = 32;
  fmt.BytesPerPixel = 4;
  fmt.Rmask = 0x00FF0000;
  fmt.Gmask = 0x0000FF00;
  fmt.Bmask = 0x000000FF;
  original = SDL_ConvertSurface(pic, &fmt, 0);
  

  std::vector<double> p; // gabor filter parameters
  p.resize(5);
  
  std::vector<double> filter; // 33x33 filter mask
  filter.resize(33*33);
  
  
  while(!exit){
    SDL_Surface* scaled = NULL;
      
    // calculates convolution of surface per each color component
    calculateGaborFilter(p, filter, 16, 0.1);
    convolveSurface(original, filter, 16, scaled); 
    
    if(ascii){
      // NOTE: ASCII rendering of images do not really work for now
      //       the code is just for testing/"visualization" purposes for now
      
      W = aa_imgwidth(ascii);
      H = aa_imgheight(ascii);
      
      // rescale picture to the target dimension
      if(pic->w < pic->h){
	double wscale = ((double)W)/((double)pic->w);
	scaled = zoomSurface(pic, wscale, wscale, SMOOTHING_ON);
			     
      }
      else{
	double hscale = ((double)H)/((double)pic->h);
	scaled = zoomSurface(pic, hscale, hscale, SMOOTHING_ON);
			     
      }
      
      // TODO convert to palette format 
      // [select 256-point centers randomly, assign pixel (r,g,b) pairs
      //  to the clusters, recalculate mean for each cluster, reassign pixel pairs again
      //  continue until convergence]
      {
	SDL_Surface* im = 
	  SDL_ConvertSurfaceFormat(scaled, SDL_PIXELFORMAT_ARGB8888, 0);
	if(im){
	  SDL_FreeSurface(scaled);
	  scaled = im;
	}
      }
      
      SDL_Palette* palette = scaled->format->palette;
      aa_palette pal;
      
      if(palette){
	for(int i=0;i<palette->ncolors;i++){
	  aa_setpalette(pal, i, 
			palette->colors[i].r, 
			palette->colors[i].g, 
			palette->colors[i].b);
	}
      }
      
      

      unsigned char* screen = (unsigned char*)aa_image(ascii);
      memset(screen, 0, W*H*sizeof(unsigned char));
      
      for(unsigned int h=0;h<(scaled->h);h++){
	for(unsigned int w=0;w<(scaled->w);w++){
	  unsigned int ww = w + (W/2) - (scaled->w/2);
	  unsigned int hh = h + (H/2) - (scaled->h/2);

	  if(palette){
	    Uint8 color = ((Uint8*)(scaled->pixels))[w + h*scaled->w];
	    unsigned char value = color;
	    
	    screen[ww + hh*W] = (unsigned char)value;
	  }
	  else{
	    Uint32 color = ((Uint32*)(scaled->pixels))[w + h*scaled->w];
	    
	    unsigned char value = (((color & 0x00FF0000)>>16) + 
				   ((color & 0x0000FF00)>>8) + 
				   ((color & 0x000000FF)>>0)) / 3;
	    
	    screen[ww + hh*W] = (unsigned char)value;
	  }
	  
	  
	}
      }
      
      SDL_FreeSurface(scaled);
      
      aa_renderparams rp;
      rp.bright    = 0; // 0-255
      rp.contrast  = 0; // 0-127
      rp.gamma     = 1.0;
      rp.dither    = AA_FLOYD_S;
      rp.inversion = 0;
      rp.randomval = 0;
      
      
      if(palette == 0){
	aa_render(ascii, &rp, 
		  0, 0, aa_scrwidth(ascii), aa_scrheight(ascii));
      }
      else{
	aa_renderpalette(ascii, pal, &rp, 
			 0, 0, aa_scrwidth(ascii), aa_scrheight(ascii));
      }
      
      // aa_fastrender(ascii, 0, 0, aa_scrwidth(ascii), aa_scrheight(ascii));
      // aa_flush(ascii);      
      
      
      aa_gotoxy(ascii, 0, 0);
      aa_hidecursor(ascii);
      unsigned char* text = aa_text(ascii);
      unsigned char* attr = aa_attrs(ascii);
      
      for(unsigned int h=0;h<aa_scrheight(ascii);h++){
	for(unsigned int w=0;w<aa_scrwidth(ascii);w++){
	  unsigned char ch = text[w + h*aa_scrwidth(ascii)];
	  unsigned char a  = attr[w + h*aa_scrwidth(ascii)];
	  if(a | AA_BOLD)    ch = ch | A_BOLD;
	  if(a | AA_DIM)     ch = ch | A_DIM;
	  if(a | AA_REVERSE) ch = ch | A_REVERSE;
	  
	  mvaddch(h, w, ch);
	}
      }
      aa_gotoxy(ascii, 0, 0);
      
      refresh();
      
      SDL_Delay(1000);
      
      // check for keypress
      int event = aa_getevent(ascii, 0);
      if(event != AA_NONE && event >= 0 && event <= 255)
	exit = true; // normal keypress
      
    }
    else{ // SDL graphics code
      // TODO implement me
      
      SDL_Surface* surf = SDL_GetWindowSurface(window);
      
      /*
       * SDL_FillRect(surf, NULL, 
       * SDL_MapRGB(surf->format, 255, 0, 0));
       */
      
      SDL_FreeSurface(surf);
      SDL_UpdateWindowSurface(window);
      SDL_ShowWindow(window);
      
      while(SDL_PollEvent(&event)){
	if(event.type == SDL_KEYDOWN)
	  exit = true;
      }
    }
    
    
  }
  

  SDL_FreeSurface(original);
  SDL_FreeSurface(pic);
  if(window) SDL_DestroyWindow(window);
  if(ascii){
    aa_close(ascii);
    endwin();
  }
  
  IMG_Quit();
  SDL_Quit();
  
  return 0;
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


