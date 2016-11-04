/*
 * MaxImpact - measure maximum reponse stimulus pictures (v1)
 *             create/modify images for maximum response (plan / v2)
 * 
 */

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include <thread>
#include <chrono>

// #include <dinrhiw.h>
// #include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <math.h>

#include <SDL.h>
#include <SDL_image.h> 

// devices that can be used for measurements
#include "DataSource.h"
#include "RandomEEG.h"
#include "MuseOSC.h"
#include "EmotivInsight.h"

// uncomment define to create command-line tool
// that cannot process muse commands.
// #define DISABLE_EEG 1

void print_show_usage(){
  printf("Usage: maximpact <device> <picture-directory> [time]\n");
  printf("       <device> = 'muse' (osc localhost udp port 4545) or 'random'\n");
  printf("\n");
}


bool parse_parameters(int argc, char** argv,
		      std::string& device,
		      std::vector<std::string>& filenames,
		      int& msecs)
{
  device = "random";

  if(argc != 3 && argc != 4) return false;

  device = argv[1];

  if(device != "muse" && device != "random" && device != "insight")
    return false;
  
  DIR *dir;
  struct dirent *ent;

  std::string path = argv[2];

  if(path.size() <= 0)
    return false;
  
  if(path[path.size()-1] == '/' || path[path.size()-1] == '\\')
    path.resize(path.size()-1);
     
  
  if ((dir = opendir (path.c_str())) != NULL) {
    
    while ((ent = readdir (dir)) != NULL) {
      const char* filename = ent->d_name;
      const int L = strlen(filename);

      if(strcmp((const char*)(filename+L-4), ".jpg") == 0 ||
	 strcmp((const char*)(filename+L-4), ".png") == 0){

	std::string f = path;
	f += "/";
	f += ent->d_name;

	// std::cout << f << std::endl;
	
	filenames.push_back(f);
      }
			    
      
    }
    closedir (dir);
  }
  else
    return false;

  msecs = 200;

  if(argc == 4)
    msecs = atoi(argv[3]);

  return ((filenames.size() > 0) && (msecs > 0));
}


double* createRandomSurface(int w, int h, double stdev);

void addDeltaToSurface(SDL_Surface* scaled, double* delta, SDL_Surface* view);

// calculates real value of gabor filter
bool calculateGaborFilter(std::vector<double>& p, 
			  std::vector<double>& filter, 
			  unsigned int N, double L);

// calculates convolution of surface per each color component
bool convolveSurface(SDL_Surface* original, 
		     std::vector<double>& filter, 
		     unsigned int N, SDL_Surface* scaled);


void cpp_sleep(int msecs)
{
  std::chrono::milliseconds duration(msecs);
  std::this_thread::sleep_for(duration);
}


void report_deltas(std::vector<std::string>& pictures,
		   std::vector< std::vector<float> >& deltas);


//////////////////////////////////////////////////////////////////////


int main(int argc, char** argv)
{
  printf("MaxImpact v0.9a Copyright Tomas Ukkonen <tomas.ukkonen@iki.fi>\n");
  
  srand(time(0));

  std::string device;
  std::vector<std::string> pictures;
  int msecs = 200; // default picture display time

  if(!parse_parameters(argc, argv, device, pictures, msecs)){
    print_show_usage();
    fprintf(stdout, "ERROR: Incorrect parameters.\n");
    return -1;
  }
  else{
    struct stat buf;
    
    for(unsigned int i=0;i<pictures.size();i++){
      if(stat(pictures[i].c_str(), &buf) != 0){
	print_show_usage();
	fprintf(stdout, "ERROR: Incorrect parameters.\n");
	return -1;    
      }
    }
  }
  

  /////////////////////////////////////////////////////////////

#ifdef DISABLE_EEG
  // always use random device
  device = "random";
#endif

  DataSource* dev = nullptr;

  if(device == "muse"){
    dev = new whiteice::resonanz::MuseOSC(4545);
  }
  else if(device == "insight")
    dev = new whiteice::resonanz::EmotivInsight();
  else if(device == "random"){
    dev = new whiteice::resonanz::RandomEEG();
  }

  cpp_sleep(2500); // waits 2.5 seconds for connection to be established.

  if(dev->connectionOk() == false){
    fprintf(stdout, "ERROR: Cannot connect to device.\n");
    return -1;    
  }
  
  
  /////////////////////////////////////////////////////////////
  
  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

  SDL_Window* window = NULL;
  
  int W = 640;
  int H = 480;


  SDL_DisplayMode mode;
  
  if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
    W = (3*mode.w)/4;
    H = (3*mode.h)/4;
  }



  std::vector<SDL_Surface*> pics;

  for(auto p : pictures){
    SDL_Surface* pic = IMG_Load(p.c_str());
    
    if(pic == NULL) break;
    
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
    
    pics.push_back(scaled);
  }

  
  if(pics.size() == 0){
    printf("Usage: maximpact <picture>\n");
    fprintf(stdout, "ERROR: Bad picture files.\n");

    for(auto p : pics)
      SDL_FreeSurface(p);
    
    delete dev;
    
    IMG_Quit();
    SDL_Quit();
    
    return -1;    
  }
  

  window = SDL_CreateWindow("MaxImpact",
			    SDL_WINDOWPOS_CENTERED,
			    SDL_WINDOWPOS_CENTERED,
			    W, H,
			    SDL_WINDOW_ALWAYS_ON_TOP |
			    SDL_WINDOW_INPUT_FOCUS);
  if(window == NULL){
    fprintf(stdout, "ERROR: Cannot open window: %s.\n", SDL_GetError()); 
    for(auto p : pics)
      SDL_FreeSurface(p);

    delete dev;
    
    IMG_Quit();
    SDL_Quit();
    
    return -1;        
  }

  // brings window on top of other windows (?)
  SDL_RaiseWindow(window);
  SDL_UpdateWindowSurface(window);
  SDL_RaiseWindow(window);
  
  
  SDL_Event event;
  bool exit = false;

  // delta values when showing each picture
  std::vector< std::vector<float> > deltas;
  deltas.resize(pictures.size());

  
  while(!exit){
    const unsigned int picIndex = rand() % pics.size();
    
    bool connOk = true;
    
    if(dev->connectionOk() == false)
      connOk = false;

    std::vector<float> before;
    if(dev->data(before) == false)
      connOk = false;
    
    {
      // SDL graphics code (TODO implement me)
      
      SDL_Surface* surf = SDL_GetWindowSurface(window);
      
      SDL_FillRect(surf, NULL, 
		   SDL_MapRGB(surf->format, 0, 0, 0));

      SDL_Surface* scaled = pics[ picIndex ];

      SDL_Rect imageRect;
      imageRect.w = scaled->w;
      imageRect.h = scaled->h;
      imageRect.x = (W - scaled->w)/2;
      imageRect.y = (H - scaled->h)/2;      

      SDL_BlitSurface(scaled, NULL, surf, &imageRect);

      
      SDL_UpdateWindowSurface(window);
      SDL_ShowWindow(window);
      SDL_FreeSurface(surf);

      // shows picture for (default: 200ms)
      cpp_sleep(msecs);
	    
      while(SDL_PollEvent(&event)){
	if(event.type == SDL_KEYDOWN)
	  exit = true;
      }
    }

    if(dev->connectionOk() == false)
      connOk = false;
    
    std::vector<float> after;
    if(dev->data(after) == false)
      connOk = false;

    if(connOk){
      float delta = 0.0f;
      
      for(unsigned int i=0;i<before.size();i++){
	delta += (after[i] - before[i])*(after[i] - before[i]);
      }
      
      delta = sqrt(delta);
      
      deltas[picIndex].push_back(delta);
    }
    
  }


  report_deltas(pictures, deltas);
  

  if(window) SDL_DestroyWindow(window);

  for(auto p : pics)
    SDL_FreeSurface(p);

  delete dev;
  
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}

//////////////////////////////////////////////////////////////////////

struct picture_delta
{
  std::string pic;
  float mean;
  float stdev;
};

int pd_cmp(const void* p11, const void* p22)
{
  const struct picture_delta *p1, *p2;
  p1 = (struct picture_delta*)p11;
  p2 = (struct picture_delta*)p22;

  if(p1->mean == p2->mean) return 0;
  else if(p1->mean > p2->mean) return -1;
  else return +1;
}



void report_deltas(std::vector<std::string>& pictures,
		   std::vector< std::vector<float> >& deltas)
{
  // create array of picture data
  struct picture_delta pics[pictures.size()];

  // for(unsigned int i=0;i<pictures.size();i++)
  // pics[i] = new struct picture_delta;

  // populate it with results
  for(unsigned int i=0;i<pictures.size();i++){
    pics[i].pic = pictures[i];
    
    std::vector<float>& data = deltas[i];

    float mean = 0.0f, var = 0.0f;
    
    for(auto d : data){
      mean += d/data.size();
      var  += d*d/data.size();
    }

    var -= mean*mean;

    pics[i].mean = mean;
    pics[i].stdev = sqrt(var);
  }
  
  // qsort it
  qsort(pics, pictures.size(), sizeof(struct picture_delta), pd_cmp);

  // prints the results
  for(unsigned int i=0;i<pictures.size();i++){
    printf("%f (+-%f)\t %s\n", pics[i].mean, pics[i].stdev,
	   pics[i].pic.c_str());
  }
  fflush(stdout);
  
}


//////////////////////////////////////////////////////////////////////

#if 0
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



void addDeltaToSurface(SDL_Surface* scaled, double* delta, SDL_Surface* view)
{
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


#endif
