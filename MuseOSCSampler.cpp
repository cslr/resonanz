
#include "MuseOSCSampler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>



static MuseOSCSampler* __sampler_instance = NULL;


void muse_error(int num, const char* m, const char *path){
  if(__sampler_instance) __sampler_instance->error(num, m, path);
}

int muse_eeg_handler(const char *path, const char *types, lo_arg ** argv,
		     int argc, void *data, void *user_data)
{
  if(__sampler_instance) 
    return __sampler_instance->eeg_handler(path, types, argv, argc, 
					   data, user_data);
}

int muse_quantization_handler(const char *path, const char *types, 
			      lo_arg ** argv,
			      int argc, void *data, void *user_data)
{
  if(__sampler_instance) 
    return __sampler_instance->quantization_handler(path, types, argv, argc, 
						    data, user_data);
}



MuseOSCSampler::MuseOSCSampler(const std::string& bind_address)
{
  __sampler_instance = this;
  
  st = lo_server_thread_new(bind_address.c_str(), muse_error);
  
  lo_server_thread_add_method(st, "/muse/eeg", "ffff", 
			      muse_eeg_handler, NULL);

  lo_server_thread_add_method(st, "/muse/eeg/quantization", "iiii", 
			      muse_quantization_handler, NULL);
}


MuseOSCSampler::~MuseOSCSampler() throw()
{
  __sampler_instance = NULL;
  lo_server_thread_free(st);
}


// handles EEG ffff OSC messages
int MuseOSCSampler::eeg_handler(const char *path, const char *types, 
				lo_arg ** argv, int argc, void *data, 
				void *user_data)
{
  int i;
  
  printf("path: <%s>\n", path);
  for (i = 0; i < argc; i++) {
    printf("arg %d '%c' ", i, types[i]);
    lo_arg_pp((lo_type)types[i], argv[i]);
    printf("\n");
  }
  printf("\n");
  fflush(stdout);
  
  // TODO: properly sample and process EEG-data
  //       store Muse EEG samples for 1 sec long buffer (220 Hz)
  
  return 0;
}


// handles EEG ffff OSC messages
int MuseOSCSampler::quantization_handler(const char *path, 
					 const char *types, 
					 lo_arg ** argv, 
					 int argc, void *data, 
					 void *user_data)
{  
  int i;
  
  printf("path: <%s>\n", path);
  for (i = 0; i < argc; i++) {
    printf("arg %d '%c' ", i, types[i]);
    lo_arg_pp((lo_type)types[i], argv[i]);
    printf("\n");
  }
  printf("\n");
  fflush(stdout);  

  // TODO: properly sample and process EEG-data
  //       store Muse EEG quantization that is always used 
  //       to multiply EEG filters
  
  return 0;
}


void MuseOSCSampler::error(int num, const char* m, const char *path)
{
  // silently ignores all errors
}

