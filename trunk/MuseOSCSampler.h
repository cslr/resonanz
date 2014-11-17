
#ifndef __MuseOSCSampler_h
#define __MuseOSCSampler_h

#include <lo/lo.h>
#include <vector>
#include <string>
#include <dinrhiw.h>


class MuseOSCSampler : public whiteice::singleton<MuseOSCSampler>
{
 public:
  MuseOSCSampler(const std::string& bind_address = "7700");
  ~MuseOSCSampler() throw();
  
 private:
  lo_server_thread st;
  
 public:
  
  // handles EEG ffff OSC messages
  int eeg_handler(const char *path, const char *types, lo_arg ** argv,
		  int argc, void *data, void *user_data);

  // handles EEG ffff OSC messages
  int quantization_handler(const char *path, 
			   const char *types, lo_arg ** argv,
			   int argc, void *data, void *user_data);
  
  // OSC error handler
  void error(int num, const char* m, const char *path);
};


#endif
