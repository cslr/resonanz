// class that scores given data, used by JNI interface

#ifndef __PredictaEngine_h
#define __PredictaEngine_h

#include <thread>
#include <string>


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
			     double risk);

      bool isActive();

      bool stopOptimization();

      std::string getError();

      std::string getStatus();
      
    private:
      std::thread workerThread;

      void loop(); // worker thread
      
    };
    
  }
  
};



#endif
