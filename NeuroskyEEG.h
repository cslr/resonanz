

#ifndef __resonanz_NeuroskyEEG_h
#define __resonanz_NeuroskyEEG_h

#include "DataSource.h"
#include <thread>
#include <mutex>
#include <stdexcept>
#include <exception>


namespace whiteice {
  namespace resonanz {
    
    class NeuroskyEEG : public DataSource {
    public:
      // listens COM5
      NeuroskyEEG() throw(std::runtime_error);
      ~NeuroskyEEG();

      /*
       * Returns unique DataSource name
       */
      virtual std::string getDataSourceName() const;
      
      /**
       * Returns true if connection and data collection 
       * to device is currently working.
       */
      virtual bool connectionOk() const;
      
      /**
       * returns current value
       */
      virtual bool data(std::vector<float>& x) const;
      
      virtual bool getSignalNames(std::vector<std::string>& names) const;
      
      virtual unsigned int getNumberOfSignals() const;

    private:
      // com port used for data transfers from neurosky
      const std::string comPort = "\\\\.\\COM5";
      
      mutable std::mutex data_mutex;
      std::vector<float> latestMeasurement;
      long long latestMeasurementTime; // timestamp in msecs for the latest measurement
      bool hasConnection;
      

      // thread that keeps polling COM6 port for
      // data forever or until polling stops
      std::thread* worker_thread;
      bool polling;

      void polling_thread(); 
      
    };
    
    
  };
};



#endif
