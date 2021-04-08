
#include "HMMStateUpdator.h"
#include <functional>


namespace whiteice
{
  namespace resonanz
  {

    
    HMMStateUpdatorThread::HMMStateUpdatorThread(whiteice::KMeans<>* kmeans,
						 whiteice::HMM* hmm,
						 std::vector< whiteice::dataset<> >* pictureData,
						 std::vector< whiteice::dataset<> >* keywordData)
    {
      this->kmeans = kmeans;
      this->hmm = hmm;
      this->pictureData = pictureData;
      this->keywordData = keywordData;

      updator_thread = nullptr;
    }
    
    
    HMMStateUpdatorThread::~HMMStateUpdatorThread()
    {
      this->stop();
    }
    
    
    bool HMMStateUpdatorThread::start()
    {
      if(kmeans == NULL || hmm == NULL ||
	 pictureData == NULL || keywordData == NULL)
	return false;

      std::lock_guard<std::mutex> lock(thread_mutex);
      
      if(thread_running){
	return false; // thread is already running
      }

      processingPicIndex = 0;
      processingKeyIndex = 0;
      thread_running = true;

      try{
	if(updator_thread){ delete updator_thread; updator_thread = nullptr; }
	updator_thread = new std::thread(std::bind(&HMMStateUpdatorThread::updator_loop, this));
      }
      catch(std::exception& e){
	thread_running = false;
	updator_thread = nullptr;
	return false;
      }

      return true;
    }
    
    bool HMMStateUpdatorThread::isRunning()
    {
      std::lock_guard<std::mutex> lock(thread_mutex);
      
      if(thread_running && updator_thread != nullptr)
	return true;
      else
	return false;
    }
    
    bool HMMStateUpdatorThread::stop()
    {
      std::lock_guard<std::mutex> lock(thread_mutex);
      
      if(thread_running == false)
	return false;
      
      thread_running = false;
      
      if(updator_thread){
	updator_thread->join();
	delete updator_thread;
      }
      
      updator_thread = nullptr;
      
      return true;
    }

    void HMMStateUpdatorThread::updator_loop()
    {

      // pictureData
      for(unsigned int p=0;p<pictureData->size();p++, processingPicIndex++)
      {
	auto& pic = (*pictureData)[p];
	
	std::vector< whiteice::dataset<>::data_normalization > norms;

	pic.getPreprocessings(0, norms);
	pic.convert(0);

	unsigned int HMMstate = hmm->sample(hmm->getPI());
	
	for(unsigned int i=0;i<pic.size(0);i++){
	  auto v = pic.access(0, i);

	  whiteice::math::vertex<> eeg;
	  eeg.resize(v.size() - hmm->getNumHiddenStates());

	  for(unsigned int j=0;j<eeg.size();j++){
	    eeg[j] = v[j];
	  }

	  unsigned int kcluster = kmeans->getClusterIndex(eeg);
	  unsigned int nextState = 0;
	  hmm->next_state(HMMstate, nextState, kcluster);
	  HMMstate = nextState;

	  for(unsigned int j=eeg.size();j<v.size();j++){
	    unsigned int index = j-eeg.size();

	    if(index == HMMstate) v[j] = 1.0f;
	    else v[j] = 0.0f;
	  }

	  pic.access(0, i) = v;
	}

	for(const auto& p : norms)
	  pic.preprocess(0, p);

	pic.preprocess(0); // always do mean-variance normalization
      }

      // keywordData
      for(unsigned int p=0;p<keywordData->size();p++, processingKeyIndex++)
      {
	auto& key = (*keywordData)[p];

	std::vector< whiteice::dataset<>::data_normalization > norms;
	
	key.getPreprocessings(0, norms);
	key.convert(0);

	unsigned int HMMstate = hmm->sample(hmm->getPI());
	
	for(unsigned int i=0;i<key.size(0);i++){
	  auto v = key.access(0, i);

	  whiteice::math::vertex<> eeg;
	  eeg.resize(v.size() - hmm->getNumHiddenStates());

	  for(unsigned int j=0;j<eeg.size();j++){
	    eeg[j] = v[j];
	  }

	  unsigned int kcluster = kmeans->getClusterIndex(eeg);
	  unsigned int nextState = 0;
	  hmm->next_state(HMMstate, nextState, kcluster);
	  HMMstate = nextState;

	  for(unsigned int j=eeg.size();j<v.size();j++){
	    unsigned int index = j-eeg.size();

	    if(index == HMMstate) v[j] = 1.0f;
	    else v[j] = 0.0f;
	  }

	  key.access(0, i) = v;
	}
	
	for(const auto& p : norms)
	  key.preprocess(0, p);

	key.preprocess(0); // always do mean-variance normalization
      }

      
      thread_running = false;
    }
    
  };
};
