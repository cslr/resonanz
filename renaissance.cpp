/*
 * Renaissance
 * 
 * (C) Copyright Tomas Ukkonen 2016
 * 
 * Read renaissance.tm for further information.
 * 
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include <string>
#include <vector>


void print_usage();
bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& picturesDir,
		      std::vector<std::string>& pictures);


int main(int argc, char** argv)
{
  printf("Renaissance 0.01 (C) Copyright Tomas Ukkonen\n");

  std::string cmd;
  std::string picturesDir;
  std::vector<std::string> pictures;

  if(!parse_parameters(argc, argv, cmd, picturesDir, pictures)){
    print_usage();
    return -1;
  }
  
  
  
  return 0;
}


void print_usage()
{
  printf("Usage: renaissance <cmd> <picture-directory>\n");
  printf("       <cmd> = \"--autoencoder\" learns autoencoder from pictures and saves it to the picture directory");
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
      
      if(strcmp((const char*)(filename+L-4), ".jpg") == 0 ||
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
