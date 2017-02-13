
#ifndef whiteice__ReinforcementSounds_h
#define whiteice__ReinforcementSounds_h


#include "DataSource.h"

#include <dinrhiw/dinrhiw.h>

#include "SDLSoundSynthesis.h"

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <SDL.h>
#include <SDL_mixer.h>

namespace whiteice
{
  namespace resonanz
  {

    template <typename T>
      class ReinforcementSounds : public whiteice::RIFL_abstract2<T>
      {
      public:
	ReinforcementSounds(const DataSource* dev,
			    const whiteice::HMM& hmm,
			    const whiteice::KMeans< T >& clusters,
			    const whiteice::bayesian_nnetwork<T>& rmodel,
			    SDLSoundSynthesis* synth, 
			    const unsigned int PLAYTIME,
			    const std::vector<double>& target,
			    const std::vector<double>& targetVar);
	
	~ReinforcementSounds();

	
	bool getSoundIsActive();
	
	// if set true use reinforcement model (nnetwork<T>) as
	// reinforcement values..
	void setReinforcementModel(bool useModel);
	
	// sets random mode (if true) instead of showing the best pic,
	// displays randomly chosen pic while doing all the calculations etc.
	void setRandom(bool r);

      protected:
	const DataSource* dev;
	whiteice::HMM hmm;
	whiteice::KMeans< T > clusters;
	whiteice::bayesian_nnetwork<T> rmodel;
	std::vector<std::string> pictures;
	unsigned int PLAYTIME;
	
	unsigned int currentHMMstate;
	
	// if true use rmodel, otherwise minimize distance ~ ||newstate - target||^2
	bool useReinforcementModel;
	
	std::vector<double> target;
	std::vector<double> targetVar;
	
	bool random; // random mode for comparing effects to user
	
	whiteice::RNG<T> rng;
	
	
	virtual bool getState(whiteice::math::vertex<T>& state);
    
	virtual bool performAction(const whiteice::math::vertex<T>& action,
				   whiteice::math::vertex<T>& newstate,
				   T& reinforcement);

	SDLSoundSynthesis* synth;
	
	
	std::list<T> distances;
	
	std::list< whiteice::math::vertex<T> > actionQueue;
	std::mutex actionMutex;
	std::condition_variable actionQueue_cond;
	
	std::list< whiteice::math::vertex<T> > performedActionsQueue;
	std::mutex performedActionsMutex;
	std::condition_variable performedActionsQueue_cond;
	
	volatile bool running;
	std::thread* audio_thread;
	std::mutex audio_mutex;
	
	void audioLoop();
	
      };


    extern template class ReinforcementSounds< whiteice::math::blas_real<float> >;
    extern template class ReinforcementSounds< whiteice::math::blas_real<double> >;
    
  };
  
};


#endif
