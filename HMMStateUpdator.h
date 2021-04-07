/*
 * HMMStateUpdatorThread
 *
 * reclassifies dataset<> classification field using K-Means and HMM model
 */

#ifndef HMMStateUpdator_h
#define HMMStateUpdator_h

#include <dinrhiw/dinrhiw.h>
#include <thread>
#include <mutex>

namespace whiteice {
  namespace resonanz {

    class HMMStateUpdatorThread
    {
    public:

      HMMStateUpdatorThread(whiteice::KMeans<>* kmeans,
			    whiteice::HMM* hmm,
			    std::vector< whiteice::dataset<> >* pictureData,
			    std::vector< whiteice::dataset<> >* keywordData);

      ~HMMStateUpdatorThread();

      bool start();
      
      bool isRunning();

      unsigned int getProcessedElements(){
	return (processingPicIndex + processingKeyIndex);
      }

      bool stop();
      
    private:
      
      void updator_loop();
      
      std::mutex thread_mutex;
      bool thread_running = false;
      std::thread* updator_thread = nullptr;
      
      whiteice::KMeans<>* kmeans;
      whiteice::HMM* hmm;
      std::vector< whiteice::dataset<> >* pictureData;
      std::vector< whiteice::dataset<> >* keywordData;

      unsigned int processingPicIndex = 0, processingKeyIndex = 0;
      
    };
    
  };
};


#endif
