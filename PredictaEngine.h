// class that scores given data, used by JNI interface

#ifndef __PredictaEngine_h
#define __PredictaEngine_h

#include <thread>
#include <string>
#include <mutex>

#include <dinrhiw.h>


namespace whiteice
{
  namespace resonanz
  {
    
    class PredictaEngine
    {
    public:
      PredictaEngine();
      ~PredictaEngine();

      bool startOptimization(const std::string& traininigFile,
			     const std::string& scoringFile,
			     const std::string& resultsFile,
			     double risk,
			     double optimizationTime,
			     bool demoVersion);

      bool isActive();

      bool stopOptimization();

      std::string getError();

      std::string getStatus();
      
    private:
      std::thread* worker_thread;
      bool running;

      void setStatus(const std::string& status);
      void setError(const std::string& error);

      std::mutex error_mutex;
      std::string latestError;

      std::mutex status_mutex;
      std::string currentStatus;

      
      std::mutex optimizeLock;
      bool optimize;
      bool thread_idle;
      
      std::string trainingFile;
      std::string scoringFile;
      std::string resultsFile;
      double risk;
      double optimizationTime;
      
      bool demoVersion;
      

      void loop(); // worker thread
      
      whiteice::dataset< whiteice::math::blas_real<double> > train;
      whiteice::dataset< whiteice::math::blas_real<double> > scoring;
      whiteice::dataset< whiteice::math::blas_real<double> > results;
      
    };
    
  }
  
};



#endif
