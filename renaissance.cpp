/*
 * Renaissance
 * 
 * (C) Copyright Tomas Ukkonen 2016
 * 
 * Read renaissance.tm for further information.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>

#include <dinrhiw/dinrhiw.h>
#include "pictureAutoencoder.h"
#include "measurements.h"
#include "optimizeResponse.h"
#include "stimulation.h"

#include "DataSource.h"
#include "RandomEEG.h"
#include "MuseOSC.h"


void print_usage();

// negative values mean given target value is ignored
bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& dir,
		      std::vector<float>& targets, 
		      std::vector<std::string>& pictures);

using namespace whiteice::resonanz;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// processes pictures of size 8x8 for now (out of memory otherwise)

#define PICTURESIZE 8
#define DISPLAYTIME 200 // picture display time in msecs



int main(int argc, char** argv)
{
  printf("Renaissance 0.02 (C) Copyright Tomas Ukkonen\n");

  srand(time(0));

  std::string cmd;
  std::string dir;
  std::vector<std::string> pictures;
  std::vector<float> targetVector;

  if(!parse_parameters(argc, argv, cmd, dir, targetVector, pictures)){
    print_usage();
    return -1;
  }

  const std::string encoderFile = dir + "/encoder.model";
  const std::string decoderFile = dir + "/decoder.model";
  const std::string datasetFile = dir + "/measurements.dat";
  const std::string modelFile   = dir + "/response.model";
  
  
  //////////////////////////////////////////////////////////////////////

  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);


  
  if(cmd == "--autoencoder"){
    // learns decoder picture synthesizer from data
    
    whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder = nullptr;
    whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder = nullptr;

    if(learnPictureAutoencoder(dir, pictures,
			       PICTURESIZE, encoder, decoder) == false){
      printf("ERROR: Optimizing autoencoder failed.\n");
      
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }
    
    
    // saves endoder and decoder into pictures directory
    {
      
      if(encoder->save(encoderFile) == false || decoder->save(decoderFile) == false){	
	printf("ERROR: cannot save encoder/decoder (autoencoder) to a disk\n");
	
	delete encoder;
	delete decoder;
	
	IMG_Quit();
	SDL_Quit();
	return -1;
      }
    }

    // testing: generates random pictures using decoder
    generateRandomPictures(decoder);

    if(encoder) delete encoder;
    if(decoder) delete decoder;
  }
  else if(cmd == "--measure" || cmd == "--test"){
    // connects to EEG hardware (Interaxon muse) and measures responses to synthesized pictures and
    // real pictures and stores results to database..
    
    DataSource* dev = nullptr;

    if(cmd == "--measure") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(cmd == "--test") dev = new whiteice::resonanz::RandomEEG();

    whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder =
      new whiteice::nnetwork< whiteice::math::blas_real<double> >();
      
    whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder =
      new whiteice::nnetwork< whiteice::math::blas_real<double> >();

    // loads decoder
    
    // loads endoder and decoder from pictures directory
    {
      if(encoder->load(encoderFile) == false || decoder->load(decoderFile) == false){	
	printf("ERROR: cannot load encoder/decoder (autoencoder) to a disk\n");
	
	delete encoder;
	delete decoder;

	delete dev;
	
	IMG_Quit();
	SDL_Quit();
	return -1;
      }
    }

    sleep(2);

    if(dev->connectionOk() == false){
      printf("ERROR: cannot connect to EEG device.\n");
      
      delete encoder;
      delete decoder;
      
      delete dev;
      
      IMG_Quit();
      SDL_Quit();
      return -1;
    }

    whiteice::dataset< whiteice::math::blas_real<double> > data;
    
    if(data.load(datasetFile) == true){ // tries to load dataset
      if(data.getNumberOfClusters() != 3){
	printf("ERROR: cannot load dataset.\n");
	
	delete encoder;
	delete decoder;
	
	delete dev;
	
	IMG_Quit();
	SDL_Quit();
	return -1;
      }
    }
      
    
    if(measureResponses(dev, DISPLAYTIME, encoder, decoder, pictures, PICTURESIZE, data) == false){
      printf("ERROR: could not measure responses to pictures\n");

      delete encoder;
      delete decoder;
      
      delete dev;
      
      IMG_Quit();
      SDL_Quit();
      return -1;
    }

    // saves dataset to disk
    {      
      if(data.save(datasetFile) == false){
	printf("ERROR: could not save measurements to disk\n");
	delete encoder;
	delete decoder;
	
	delete dev;
	
	IMG_Quit();
	SDL_Quit();
	return -1;
      }

      if(data.getNumberOfClusters() > 0)
	printf("Total %d measurements in database\n", data.size(0));
    }

    delete encoder;
    delete decoder;
    delete dev;
  }
  else if(cmd == "--optimize"){
    // optimizes measurements using neural networks: nn(picparams, current_state) = next_state
    
    whiteice::nnetwork< whiteice::math::blas_real<double> >* nn = nullptr;
    
    whiteice::dataset< whiteice::math::blas_real<double> > data;
    
    if(data.load(datasetFile) == true){ // tries to load dataset
      if(data.getNumberOfClusters() != 3){
	printf("ERROR: cannot load dataset.\n");
	IMG_Quit();
	SDL_Quit();
	return -1;
      }
    }

    // creates and optimizes neural network using L-BFGS training:
    // nn(picparams, current_eeg_state) = next_eeg_state
    if(optimizeResponse(nn, data) == false){
      printf("ERROR: cannot load response dataset.\n");
      IMG_Quit();
      SDL_Quit();
      return -1;
    }

    if(nn->save(modelFile) == false){
      printf("ERROR: Cannot save response model to disk\n");
      delete nn;
      IMG_Quit();
      SDL_Quit();
      return -1;
    }
    else{
      printf("NN prediction model saved.\n");
    }

    delete nn;
  }
  else if(cmd == "--synthesize"){ // decoder(params) = picture
    // loads prediction model and optimizes picparams using model nn(picparams, current_state) = next_state
    // (initially we do random search of picparams and always select the best one found)

    whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder =
      new whiteice::nnetwork< whiteice::math::blas_real<double> >();
    
    // loads decoder
    
    // loads decoder from pictures directory
    {
      if(decoder->load(decoderFile) == false){
	printf("ERROR: cannot load decoder (autoencoder) from a disk\n");
	
	delete decoder;
	
	IMG_Quit();
	SDL_Quit();
	return -1;
      }
    }

    whiteice::nnetwork< whiteice::math::blas_real<double> >* nn =
      new whiteice::nnetwork< whiteice::math::blas_real<double> >();
    
    if(nn->load(modelFile) == false){
      printf("ERROR: Cannot load response model from disk\n");
      delete nn;
      delete decoder;
      IMG_Quit();
      SDL_Quit();
      return -1;
    }

    whiteice::math::vertex< whiteice::math::blas_real<double> > target;
    target.resize(targetVector.size());

    for(unsigned int i=0;i<target.size();i++)
      target[i] = targetVector[i];
    
    
    if(synthStimulation(decoder, PICTURESIZE, DISPLAYTIME, nn, target) == false){
      printf("ERROR: picture synthesizer stimulation failed\n");
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    
  }
  else if(cmd == "--stimulate"){ // encoder(picture) = params
    // goes through all pictures and always selects picture with best nn(picparams, current_state) = next_state

    // There is some problems using encoder

    // TODO/FIXME
  }

  
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


void print_usage()
{
  printf("Usage: renaissance <cmd> <directory> [--target=0.0,0.0,0.0,0.0,0.0,0.0]\n");
  printf("       <cmd> = \"--autoencoder\" learns autoencoder from png-files in directory and saves autoencoder\n");
  printf("       <cmd> = \"--measure\"     stimulate cns using decoder and stores responses to dataset file\n");
  printf("                               reads responses from interaxon muse (osc.udp://localhost:4545/)\n");
  printf("       <cmd> = \"--test\"        stimulate cns using decoder and generate random measurements\n");
  printf("       <cmd> = \"--optimize\"    optimizes model using dataset file\n");
  printf("       <cmd> = \"--synthesize\"  uses model and decoder to push brain towards target\n");
  printf("       <cmd> = \"--stimulate\"   uses model, pictures in directory and encoder to push brain towards target state\n");
  printf("       <directory>             path to directory (png and model files)\n");
  printf("       --target=<vector>       optimization target [0,1]^6 vector. (?) value means value can be anything\n");
  printf("                               <vector>= delta, theta, alpha, beta, gamma, total power scaled within [0,1]\n");
  printf("\n");
}


bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& dir,
		      std::vector<float>& targets, // negative values mean given target value is ignored
		      std::vector<std::string>& pictures)
{
  if(argc != 3 && argc != 4)
    return false;

  cmd = argv[1];
  dir = argv[2];
  pictures.clear();
  targets.clear();

  if(cmd != "--autoencoder" &&
     cmd != "--measure" &&
     cmd != "--test" &&
     cmd != "--optimize" &&
     cmd != "--stimulate")
    return false;

  if(argc == 4 && (cmd != "--stimulate" && cmd != "--synthesize"))
    return false;

  if(cmd == "--stimulate" || cmd == "--synthesize"){
    if(strncmp(argv[3], "--target=", 9) == 0){
      char* v = argv[3] + 9;
      char* ptr = NULL;
      char* token = strtok_r(v, ",", &ptr);

      targets.push_back(atof(token));

      while((token = strtok_r(NULL, ",", &ptr)) != NULL){
	if(strcmp(token, "?") == 0)
	  targets.push_back(-1.0);
	else
	  targets.push_back(atof(token));
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
      
      if(/*strcmp((const char*)(filename+L-4), ".jpg") == 0 || */
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
