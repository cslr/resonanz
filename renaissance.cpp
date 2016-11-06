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


void print_usage();
bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& picturesDir,
		      std::vector<std::string>& pictures);

using namespace whiteice::resonanz;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// processes pictures of size 8x8 for now (out of memory otherwise)

#define PICTURESIZE 8


int main(int argc, char** argv)
{
  printf("Renaissance 0.02 (C) Copyright Tomas Ukkonen\n");

  srand(time(0));

  std::string cmd;
  std::string picturesDir;
  std::vector<std::string> pictures;

  if(!parse_parameters(argc, argv, cmd, picturesDir, pictures)){
    print_usage();
    return -1;
  }

  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

  if(cmd == "--autoencoder"){
    whiteice::nnetwork< whiteice::math::blas_real<double> >* encoder = nullptr;
    whiteice::nnetwork< whiteice::math::blas_real<double> >* decoder = nullptr;

    if(learnPictureAutoencoder(picturesDir, pictures,
			       PICTURESIZE, encoder, decoder) == false){
      printf("ERROR: Optimizing autoencoder failed.\n");
      
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }
    
    // saves endoder and decoder into pictures directory
    {
      std::string encoderFile = picturesDir + "/encoder.model";
      std::string decoderFile = picturesDir + "/decoder.model";
      
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

  
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


void print_usage()
{
  printf("Usage: renaissance <cmd> <picture-directory>\n");
  printf("       <cmd> = \"--autoencoder\" learns autoencoder from pictures and saves it to the picture directory\n");
  printf("       <picture-directory> path to directory (png files)\n");
  printf("\n");
}


bool parse_parameters(int argc, char** argv, std::string& cmd, std::string& picturesDir, std::vector<std::string>& pictures)
{
  if(argc != 3) return false;

  cmd = argv[1];
  picturesDir = argv[2];
  pictures.clear();

  if(cmd != "--autoencoder") return false;
  
  DIR *dir;
  struct dirent *ent;

  std::string& path = picturesDir;

  if(path.size() <= 0)
    return false;
  
  if(path[path.size()-1] == '/' || path[path.size()-1] == '\\')
    path.resize(path.size()-1);
  
  
  if ((dir = opendir (path.c_str())) != NULL) {
    
    while ((ent = readdir (dir)) != NULL) {
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
    closedir (dir);
  }
  else return false;

  if(pictures.size() <= 0)
    return false;
  
  return true;
}
