/*
 * MuseOSC.cpp
 *
 *  Created on: 7.7.2015
 *      Author: Tomas
 */

#include "MuseOSC.h"
#include <math.h>
#include <unistd.h>
#include <chrono>


#include "oscpkt.hh"
#include "udp.hh"

using namespace oscpkt;
using namespace std::chrono;

namespace whiteice {
namespace resonanz {

MuseOSC::MuseOSC(unsigned int portNum) throw(std::runtime_error) :
		port(portNum)
{
  worker_thread = nullptr;
  running = false;
  hasConnection = false;
  quality = 0.0f;
  
  value.resize(this->getNumberOfSignals());
  for(auto& v : value) v = 0.0f;
  
  latest_sample_seen_t = 0LL;
  
  try{
    std::unique_lock<std::mutex> lock(connection_mutex);
    running = true;
    worker_thread = new std::thread(&MuseOSC::muse_loop, this);

    // waits for up to 5 seconds for connection to be established
    // or silently fails (device has no connection when accessed)

    std::chrono::system_clock::time_point now =
      std::chrono::system_clock::now();
    auto end_time = now + std::chrono::seconds(5);

    while(hasConnection == true || std::chrono::system_clock::now() >= end_time)
      connection_cond.wait_until(lock, end_time);
  }
  catch(std::exception){
    running = false;
    throw std::runtime_error("MuseOSC: couldn't create worker thread.");
  }
}


MuseOSC::~MuseOSC()
{
  running = false;
  if(worker_thread != nullptr)
    worker_thread->join();
  
  worker_thread = nullptr;
}

/*
 * Returns unique DataSource name
 */
std::string MuseOSC::getDataSourceName() const
{
  return "Interaxon Muse";
}

/**
 * Returns true if connection and data collection to device is currently working.
 */
bool MuseOSC::connectionOk() const
{
  if(hasConnection == false)
    return false; // there is no connection
  
  long long ms_since_epoch = (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  
  if((ms_since_epoch - latest_sample_seen_t) > 2000)
    return false; // latest sample is more than 2000ms second old => bad connection/data
  else
    return true; // sample within 2000ms => good
}

/**
 * returns current value
 */
bool MuseOSC::data(std::vector<float>& x) const
{
  std::lock_guard<std::mutex> lock(data_mutex);
  
  if(this->connectionOk() == false)
    return false;
  
  x = value;
  
  return true;
}

bool MuseOSC::getSignalNames(std::vector<std::string>& names) const
{
  names.resize(6);

  // delta, theta, alpha, beta, gamma
  
  names[0] = "Muse: Delta";
  names[1] = "Muse: Theta";
  names[2] = "Muse: Alpha";
  names[3] = "Muse: Beta";
  names[4] = "Muse: Gamma";
  names[5] = "Muse: Total Power";
  
  return true;
}

unsigned int MuseOSC::getNumberOfSignals() const
{
  return 6;
}

void MuseOSC::muse_loop() // worker thread loop
{
  // sets MuseOSC internal thread high priority thread
  // so that connection doesn't timeout
  {
    sched_param sch_params;
    int policy = SCHED_FIFO; // SCHED_RR
    
    pthread_getschedparam(pthread_self(), &policy, &sch_params);
    
    policy = SCHED_FIFO;
    sch_params.sched_priority = sched_get_priority_max(policy);
    
    if(pthread_setschedparam(pthread_self(),
			     policy, &sch_params) != 0){
    }
    
#ifdef WINOS
    SetThreadPriority(GetCurrentThread(),
		      THREAD_PRIORITY_HIGHEST);
    //SetThreadPriority(GetCurrentThread(),
    //THREAD_PRIORITY_TIME_CRITICAL);
#endif
  }
  
  hasConnection = false;
  
  UdpSocket sock;
  
  while(running){
    sock.bindTo(port);
    if(sock.isOk()) break;
    sock.close();
    sleep(1);
  }
  
  PacketReader pr;
  PacketWriter pw;
  
  std::vector<int> connectionQuality;
  
  float delta = 0.0f, theta = 0.0f, alpha = 0.0f, beta = 0.0f, gamma = 0.0f;
  bool hasNewData = false;
  
  while(running){
    
    if(hasConnection && hasNewData){ // updates data
      float q = 0.0f;
      for(auto qi : connectionQuality)
	if(qi > 0) q++;
      q = q / connectionQuality.size();
      
      quality = q;
      
      std::vector<float> v;
      
      
      // converts absolute power (logarithmic bels) to [0,1] value by saturating
      // values using tanh(t) this limits effective range of the values to [-0.1, 1.2]
      // TODO: calculate statistics of delta, theta, alpha, beta, gamma to optimally saturate values..
      
      auto t = delta; t = (1 + tanh(2*(t - 0.6)))/2.0;
      v.push_back(t);
      
      t = theta; t = (1 + tanh(2*(t - 0.6)))/2.0;
      v.push_back(t);
      
      t = alpha; t = (1 + tanh(2*(t - 0.6)))/2.0;
      v.push_back(t);
      
      t = beta; t = (1 + tanh(2*(t - 0.6)))/2.0;
      v.push_back(t);
      
      t = gamma; t = (1 + tanh(2*(t - 0.6)))/2.0;
      v.push_back(t);
      
      // calculates total power in decibels [sums power terms together]
      float total = pow(10.0f, delta/10.0f) + pow(10.0f, theta/10.0f) + pow(10.0f, alpha/10.0f) + pow(10.0f, beta/10.0f) + pow(10.0f, gamma/10.0f);
      total = 10.0f * log10(total);
      
      
      t = total; t = (1 + tanh(2*(t - 7.0)))/2.0;
      v.push_back(t);
      
      // gets current time
      auto ms_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      
      std::lock_guard<std::mutex> lock(data_mutex);
      value = v;
      latest_sample_seen_t = (long long)ms_since_epoch;
      
      // printf("EEQ: D:%.2f T:%.2f A:%.2f B:%.2f G:%.2f [QUALITY: %.2f]\n", delta, theta, alpha, beta, gamma, q);
      // printf("EEQ POWER: %.2f [QUALITY %.2f]\n", log(exp(delta)+exp(theta)+exp(alpha)+exp(beta)+exp(gamma)), q);
      // fflush(stdout);
      
      hasNewData = false; // this data point has been processed
    }
    
    if(sock.receiveNextPacket(30)){
      
      pr.init(sock.packetData(), sock.packetSize());
      Message* msg = NULL;
      
      while(pr.isOk() && ((msg = pr.popMessage()) != 0)){
	Message::ArgReader r = msg->match("/muse/elements/is_good");
	
	if(r.isOk() == true){ // matched
	  // there are 4 ints telling connection quality
	  std::vector<int> quality;
	  
	  while(r.nbArgRemaining()){
	    if(r.isInt32()){
	      int32_t i;
	      r = r.popInt32(i);
	      quality.push_back((int)i);
	    }
	    else{
	      r = r.pop();
	    }
	  }
	  
	  if(quality.size() > 0){
	    connectionQuality = quality;
	  }
	  
	  bool connection = false;
	  
	  for(auto q : quality)
	    if(q > 0) connection = true;

	  {
	    std::lock_guard<std::mutex> lock(connection_mutex);
	    hasConnection = connection;
	    connection_cond.notify_all();
	  }
	}
	
	// gets relative frequency bands powers..
	std::vector<float> f(connectionQuality.size());
	
	r = msg->match("/muse/elements/delta_absolute");
	if(r.isOk()){
	  if(r.popFloat(f[0]).popFloat(f[1]).popFloat(f[2]).popFloat(f[3]).isOkNoMoreArgs()){
	    std::vector<float> samples;
	    for(unsigned int i=0;i<f.size();i++)
	      if(connectionQuality[i]) samples.push_back(f[i]);
	    
	    float mean = 0.0f;
	    for(auto& si :samples) mean += pow(10.0f, si/10.0f);
	    mean /= samples.size();
	    
	    delta = 10.0f * log10(mean);
	    hasNewData = true;
	  }
	  
	}
	
	r = msg->match("/muse/elements/theta_absolute");
	if(r.isOk()){
	  if(r.popFloat(f[0]).popFloat(f[1]).popFloat(f[2]).popFloat(f[3]).isOkNoMoreArgs()){
	    std::vector<float> samples;
	    for(unsigned int i=0;i<f.size();i++)
	      if(connectionQuality[i]) samples.push_back(f[i]);
	    
	    float mean = 0.0f;
	    for(auto& si :samples) mean += pow(10.0f, si/10.0f);
	    mean /= samples.size();
	    
	    theta = 10.0f * log10(mean);
	    hasNewData = true;
	  }
	}
	
	r = msg->match("/muse/elements/alpha_absolute");
	if(r.isOk()){
	  if(r.popFloat(f[0]).popFloat(f[1]).popFloat(f[2]).popFloat(f[3]).isOkNoMoreArgs()){
	    std::vector<float> samples;
	    for(unsigned int i=0;i<f.size();i++)
	      if(connectionQuality[i]) samples.push_back(f[i]);
	    
	    float mean = 0.0f;
	    for(auto& si :samples) mean += pow(10.0f, si/10.0f);
	    mean /= samples.size();
	    
	    alpha = 10.0f * log10(mean);
	    hasNewData = true;
	  }
	}
	
	r = msg->match("/muse/elements/beta_absolute");
	if(r.isOk()){
	  if(r.popFloat(f[0]).popFloat(f[1]).popFloat(f[2]).popFloat(f[3]).isOkNoMoreArgs()){
	    std::vector<float> samples;
	    for(unsigned int i=0;i<f.size();i++)
	      if(connectionQuality[i]) samples.push_back(f[i]);
	    
	    float mean = 0.0f;
	    for(auto& si :samples) mean += pow(10.0f, si/10.0f);
	    mean /= samples.size();
	    
	    beta = 10.0f * log10(mean);
	    hasNewData = true;
	  }
	}
	
	r = msg->match("/muse/elements/gamma_absolute");
	if(r.isOk()){
	  if(r.popFloat(f[0]).popFloat(f[1]).popFloat(f[2]).popFloat(f[3]).isOkNoMoreArgs()){
	    std::vector<float> samples;
	    for(unsigned int i=0;i<f.size();i++)
	      if(connectionQuality[i]) samples.push_back(f[i]);
	    
	    float mean = 0.0f;
	    for(auto& si :samples) mean += pow(10.0f, si/10.0f);
	    mean /= samples.size();
	    
	    gamma = 10.0f * log10(mean);
	    hasNewData = true;
	  }
	}
	
      }
    }
    
    
    if(sock.isOk() == false){
      // tries to reconnect the socket to port
      sock.close();
      sleep(1);
      sock.bindTo(port);
    }
    
  }
  
  sock.close();
}
  
} /* namespace resonanz */
} /* namespace whiteice */
