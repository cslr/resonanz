
#include "NeuroskyEEG.h"
#include <assert.h>

#include <unistd.h>
#include <chrono>
#include <exception>

#include "thinkgear.h"


using namespace std::chrono;


namespace whiteice
{
  namespace resonanz
  {
    
    NeuroskyEEG::NeuroskyEEG() throw(std::runtime_error)
    {
      worker_thread = nullptr;
      polling = false;
      hasConnection = false;

      latestMeasurement.resize(this->getNumberOfSignals());
      for(auto& v : latestMeasurement) v = 0.0f;

      latestMeasurementTime = 0LL;

      try{
	polling = true;
	worker_thread = new std::thread(&NeuroskyEEG::polling_thread, this);
      }
      catch(std::exception){
	polling = false;
	throw std::runtime_error("NeuroskyEEG: couldn't create worker thread.");
      }
      
    }

    
    NeuroskyEEG::~NeuroskyEEG()
    {
      polling = false;
      if(worker_thread != nullptr)
	worker_thread->join();
      
    }
    
    /*
     * Returns unique DataSource name
     */
    std::string NeuroskyEEG::getDataSourceName() const
    {
      return "NeuroskyEEG";
    }
    
    /**
     * Returns true if connection and data collection 
     * to device is currently working.
     */
    bool NeuroskyEEG::connectionOk() const
    {
      if(hasConnection == false) return false; // no connection to HW

      long long ms_since_epoch =
	(long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

      if((ms_since_epoch - latestMeasurementTime) > 1000)
	return false; // no new data within 1 sec
      else
	return true;
    }
      
    /**
     * returns current value
     */
    bool NeuroskyEEG::data(std::vector<float>& x) const
    {
      if(this->connectionOk() == false)
	return false;

      std::lock_guard<std::mutex> lock(data_mutex);
      
      x = latestMeasurement;

      return true;
    }

    
    bool NeuroskyEEG::getSignalNames(std::vector<std::string>& names) const
    {
      names.resize(2);

      names[0] = "NeuroSky: Attention";
      names[1] = "NeuroSky: Meditation";

      return true;
    }

    
    unsigned int NeuroskyEEG::getNumberOfSignals() const
    {
      return 2;
    }

    //////////////////////////////////////////////////////////////////////

    void NeuroskyEEG::polling_thread()
    {
      hasConnection = false;

      int dllVersion = TG_GetVersion();
      int connectionId = -1;
      int errCode = 0;

      // established connection
      while(polling){
	if(connectionId < 0)
	  connectionId = TG_GetNewConnectionId();
	   
	if(connectionId < 0){
	  usleep(100000); // 100ms
	  continue;
	}
	else{
	  // we got a connection..

	  errCode = TG_Connect( connectionId,
				comPort.c_str(),
				TG_BAUD_57600,
				TG_STREAM_PACKETS );

	  if(errCode < 0){
	    usleep(100000); // 100ms
	    continue;
	  }
	  else break; // we got full connection to NeuroSky
	}
      }

      hasConnection = true;
      
      while(polling){
	
	while(TG_ReadPackets(connectionId, 1) == 1){
	  float attention = 0.0f;
	  bool hasAttention = false;
	  
	  float meditation = 0.0f;
	  bool hasMeditation = false;

	  if(TG_GetValueStatus(connectionId, TG_DATA_ATTENTION) != 0){
	    hasAttention = true;
	    attention = TG_GetValue(connectionId, TG_DATA_ATTENTION);
	  }

	  if(TG_GetValueStatus(connectionId, TG_DATA_MEDITATION) != 0){
	    hasMeditation = true;
	    meditation = TG_GetValue(connectionId, TG_DATA_MEDITATION);
	  }
	  
	  // we now has new values so we update measurements
	  if(hasAttention || hasMeditation){
	    long long ms_since_epoch =
	      (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	    std::lock_guard<std::mutex> lock(data_mutex);

	    latestMeasurementTime = ms_since_epoch;

	    if(hasAttention)
	      latestMeasurement[0] = attention;

	    if(hasMeditation)
	      latestMeasurement[1] = meditation;
	  }
	}


	// tries to reset connection if there are no packets within 1000 msecs
	{
	  long long ms_since_epoch =
	    (long long)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	  if(ms_since_epoch - latestMeasurementTime > 1000){
	    hasConnection = false;
	    
	    TG_Disconnect(connectionId);
	    TG_FreeConnection( connectionId );
	    connectionId = -1;

	    // tries to re-establish connection
	    while(polling){
	      if(connectionId < 0)
		connectionId = TG_GetNewConnectionId();
	   
	      if(connectionId < 0){
		usleep(100000); // 100ms
		continue;
	      }
	      else{
		// we got a connection..
		
		errCode = TG_Connect( connectionId,
				      comPort.c_str(),
				      TG_BAUD_57600,
				      TG_STREAM_PACKETS );
		
		if(errCode < 0){
		  usleep(100000); // 100ms
		  continue;
		}
		else break; // we got full connection to NeuroSky
	      }
	    }
	    
	    hasConnection = true;
	  }
	}
	
	
      }
      
      hasConnection = false;

      TG_Disconnect(connectionId);
      TG_FreeConnection( connectionId );
    }
    
  };
};
