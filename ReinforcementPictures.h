

#ifndef whiteice_ReinforcementPictures_h
#define whiteice_ReinforcementPictures_h

#include "ts_measure.h"

#include "DataSource.h"

#include <dinrhiw/dinrhiw.h>

#include <vector>
#include <thread>
#include <mutex>


namespace whiteice
{
  
  template <typename T>
  class ReinforcementPictures : public whiteice::RIFL_abstract<T>
  {
  public:
    ReinforcementPictures(const DataSource* dev,
			  const whiteice::HMM& hmm,
			  const whiteice::KMeans< T >& clusters,
			  const whiteice::bayesian_nnetwork<T>& rmodel,
			  const std::vector<std::string>& pictures,
			  const unsigned int DISPLAYTIME,
			  const std::vector<double>& target,
			  const std::vector<double>& targetVar);
    
    ~ReinforcementPictures();

    bool getKeypress();

    bool getDisplayIsRunning();

    // sets random mode (if true) instead of showing the best pic,
    // displays randomly chosen pic while doing all the calculations etc.
    void setRandom(bool r);
    
  protected:
    const DataSource* dev;
    whiteice::HMM hmm;
    whiteice::KMeans< T > clusters;
    whiteice::bayesian_nnetwork<T> rmodel;
    std::vector<std::string> pictures;
    unsigned int DISPLAYTIME;

    unsigned int currentHMMstate;
    
    std::vector<double> target;
    std::vector<double> targetVar;

    bool random; // random mode for comparing effects to user

    whiteice::RNG<T> rng;

    std::string fontname;
    
    
    virtual bool getState(whiteice::math::vertex<T>& state);
    
    virtual bool performAction(const unsigned int action,
			       whiteice::math::vertex<T>& newstate,
			       T& reinforcement);

    std::list<T> distances;

    std::list<unsigned int> actionQueue;
    std::mutex actionMutex;

    std::list<unsigned int> performedActionsQueue;
    std::mutex performedActionsMutex;
    
    volatile bool running;
    std::thread* display_thread;
    std::mutex display_mutex;
    
    unsigned int keypresses;
    
    void displayLoop();
  };
  
  
  extern template class ReinforcementPictures< math::blas_real<float> >;
  extern template class ReinforcementPictures< math::blas_real<double> >;
};



#endif
