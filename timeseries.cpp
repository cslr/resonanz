/*
 * Time Series Analysis of EEG using HMM
 *
 * (C) Copyright Tomas Ukkonen 2017
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <dinrhiw/dinrhiw.h>

#include <SDL.h>
#include <SDL_image.h>

#include <dinrhiw/dinrhiw.h>

#include <vector>
#include <string>

#include "DataSource.h"
#include "RandomEEG.h"
#include "MuseOSC.h"

#include "ts_measure.h"

void print_usage();

bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& device,
		      std::string& dir,
		      std::vector<float>& targets, 
		      std::vector<float>& targetVar,
		      std::vector<std::string>& pictures);


#define DISPLAYTIME 500 // picture display time in msecs


using namespace whiteice::resonanz;


int main(int argc, char** argv)
{
  printf("Time Series Analysis (HMM) 0.01 (C) Copyright Tomas Ukkonen\n");

  srand(time(0));
  
  std::string cmd;
  std::string device;
  std::string dir;
  std::vector<std::string> pictures;
  std::vector<float> targetVector;
  std::vector<float> targetVar;

  if(!parse_parameters(argc, argv, cmd, device, dir, targetVector, targetVar, pictures)){
    print_usage();
    return -1;
  }

  char buffer[128];

  sprintf(buffer, "%s/%s-measurements.dat", dir.c_str(), device.c_str());
  const std::string timeseriesFile = buffer;
  
  sprintf(buffer, "%s/%s-hmm.model", dir.c_str(), device.c_str());
  const std::string hmmFile     = buffer;

  //////////////////////////////////////////////////////////////////////

  if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
    printf("ERROR: SDL initialization failed: %s\n", SDL_GetError());
    return -1;
  }

  const unsigned int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    
  if(IMG_Init(img_flags) != img_flags){
    printf("ERROR: SDL IMAGE initialization failed\n");
    return -1;
  }
  
  //////////////////////////////////////////////////////////////////////

  if(cmd == "--measure1"){
    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    whiteice::dataset< whiteice::math::blas_real<double> > timeseries;

    timeseries.load(timeseriesFile); // attempts to load previously measured time-series
    
    if(measureRandomPicturesAndTimeSeries(dev, pictures,
					  DISPLAYTIME, timeseries) == false)
    {
      printf("ERROR: measuring time series failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    if(timeseries.save(timeseriesFile) == false){
      printf("ERROR: saving time series measurements to file failed\n");
      
      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    printf("Saving %d step length time series observation..\n", timeseries.size(0));

    delete dev;
  }
  else if(cmd == "--learn"){

    whiteice::dataset< whiteice::math::blas_real<double> > timeseries;

    if(timeseries.load(timeseriesFile) == false){
      printf("ERROR: loading time series measurements from file failed\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    
    const unsigned int VISIBLE_SYMBOLS = pow(3, dev->getNumberOfSignals());
    const unsigned int HIDDEN_STATES   = 5;
    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    std::vector<unsigned int> observations;

    // discretizes observations
    std::vector<double> m, s;

    // calculates parameters (mean and standard deviation)

    m.resize(dev->getNumberOfSignals());
    s.resize(dev->getNumberOfSignals());

    for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
      m[i] = 0.0;
      s[i] = 0.0;
    }
    
    for(unsigned int i=0;i<timeseries.size(0);i++){
      auto& v = timeseries[i];
      
      for(unsigned int k=0;k<dev->getNumberOfSignals();k++){
	m[k] += v[k].c[0];
	s[k] += v[k].c[0]*v[k].c[0];
      }
    }
    
    for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
      m[i] /= ((double)timeseries.size(0));
      s[i] /= ((double)timeseries.size(0));

      s[i] = m[i]*m[i];
      s[i] = sqrt(fabs(s[i]));
    }

    // DISCRETIZATION
    
    for(unsigned int i=0;i<timeseries.size(0);i++){
      auto& v = timeseries[i];

      unsigned int state = 0;

      std::vector<unsigned int> o;

      for(unsigned int k=0;k<dev->getNumberOfSignals();k++){
	const auto& x = v[k].c[0];

	if(fabs(x - m[k]) <= 0.5*s[k]){ // within 34% of the mean
	  o.push_back(0);
	}
	else if(x - m[k] <= -0.5*s[k]){
	  o.push_back(1);
	}
	else{ // (x - m[k] >= +0.5*s[k])
	  o.push_back(2);
	}
      }

      unsigned int base = 1;

      for(unsigned int k=0;k<o.size();k++){
	state += o[k]*base;
	base = base*3;
      }

      observations.push_back(state);
    }
      
    // learns from data
    try{
      double logp = hmm.train(observations);

      printf("DATA LIKELIHOOD: %f\n", logp);
      
    }
    catch(std::invalid_argument& e){
      printf("ERROR: learning from HMM data failed: %s\n", e.what());

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    if(hmm.save(hmmFile) == false){
      printf("ERROR: saving HMM model failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    return 0;
  }
  else if(cmd == "--predict"){
    
    whiteice::dataset< whiteice::math::blas_real<double> > timeseries;

    if(timeseries.load(timeseriesFile) == false){
      printf("ERROR: loading time series measurements from file failed\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    const unsigned int VISIBLE_SYMBOLS = pow(3, dev->getNumberOfSignals());
    const unsigned int HIDDEN_STATES   = 5;
    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    if(hmm.load(hmmFile) == false){
      printf("ERROR: loading Hidden Markov Model (HMM) failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    if(predictHiddenState(dev, hmm, pictures,
			  DISPLAYTIME, timeseries) == false)
    {
      printf("ERROR: measuring time series failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }


    
    return -1;
  }
  else if(cmd == "--stimulate"){
    return -1;
  }

  
  
  return 0;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


void print_usage()
{
  printf("Usage: timeseries <cmd> <device> <directory> [--target=0.0,0.0,0.0,0.0,0.0,0.0]\n");
  printf("       <cmd> = \"--measure1\"    stimulates cns randomly and collects time-series measurements\n");
  printf("       <cmd> = \"--learn\"       optimizes HMM model using measurements and optimizes neural network models using input and additional hidden states\n");
  printf("       <cmd> = \"--predict\"     predicts and outputs currently predicted hidden state\n");
  printf("       <cmd> = \"--measure2\"    stimulates cns randomly and collects picture-wise information\n"); 
  printf("       <cmd> = \"--stimulate\"   uses model, pictures in directory and neural network models to push brain towards target state\n");
  printf("       <device>                'muse' (interaxon muse osc.udp://localhost:4545) or 'random' pseudorandom measurements\n");
  printf("       <directory>             path to directory (png and model files)\n");
  printf("       --target=<vector>       optimization target [0,1]^6 vector. (?) value means value can be anything\n");
  printf("                               <vector>= delta, theta, alpha, beta, gamma, total power scaled within [0,1]\n");
  printf("\n");
}


bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& device,
		      std::string& dir,
		      std::vector<float>& targets, // negative values mean given target value is ignored
		      std::vector<float>& targetVar,
		      std::vector<std::string>& pictures)
{
  if(argc != 4 && argc != 5)
    return false;

  cmd = argv[1];
  device = argv[2];
  dir = argv[3];
  pictures.clear();
  targets.clear();

  if(cmd != "--measure1" &&
     cmd != "--learn" &&
     cmd != "--predict" &&
     cmd != "--measure2" && 
     cmd != "--stimulate")
    return false;

  if(device != "muse" &&
     device != "random")
    return false;

  if(argc != 5 && (cmd == "--stimulate"))
    return false;

  if(cmd == "--stimulate"){
    if(strncmp(argv[4], "--target=", 9) == 0){
      char* v = argv[4] + 9;
      char* ptr = NULL;
      char* token = strtok_r(v, ",", &ptr);

      targets.push_back(atof(token));
      if(strcmp(token, "?") == 0){
	targetVar.push_back(10e16);
      }
      else{
	targetVar.push_back(1.0);
      }

      while((token = strtok_r(NULL, ",", &ptr)) != NULL){
	if(strcmp(token, "?") == 0){
	  targets.push_back(0.0);
	  targetVar.push_back(10e16); // very large value for this dimension
	}
	else{
	  targets.push_back(atof(token));
	  targetVar.push_back(1.0);
	}
      }
    }
    else
      return false; // unknown parameter

    if(targets.size() != 6)
      return false; // there should be six tokens in targets (interaxon muse)
  }

  DIR *dirh;
  struct dirent *ent;

  std::string& path = dir;

  if(path.size() <= 0)
    return false;
  
  if(path[path.size()-1] == '/' || path[path.size()-1] == '\\')
    path.resize(path.size()-1);
  
  
  if ((dirh = opendir (path.c_str())) != NULL) {
    
    while ((ent = readdir (dirh)) != NULL) {
      const char* filename = ent->d_name;
      const int L = strlen(filename);
      
      if(/*strcmp((const char*)(filename+L-4), ".jpg") == 0 ||*/ 
	 strcmp((const char*)(filename+L-4), ".png") == 0){
	
	std::string f = path;
	f += "/";
	f += ent->d_name;
	
	// std::cout << f << std::endl;
	
	pictures.push_back(f);
      }
      
      
    }
    closedir (dirh);
  }
  else return false;

  if(pictures.size() <= 0)
    return false;

  return true;
}
