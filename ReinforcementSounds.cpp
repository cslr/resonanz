
#include "ReinforcementSounds.h"

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <chrono>

#include <SDL.h>

using namespace std::chrono_literals;
using namespace std::chrono;


namespace whiteice
{
  namespace resonanz
  {

    template <typename T>
    ReinforcementSounds<T>::ReinforcementSounds
    (const DataSource* dev,
     const whiteice::HMM& hmm,
     const whiteice::KMeans< T >& clusters,
     const whiteice::bayesian_nnetwork<T>& rmodel,
     SoundSynthesis* synth,
     const unsigned int PLAYTIME,
     const std::vector<double>& target,
     const std::vector<double>& targetVar) :
      whiteice::RIFL_abstract2<T>(synth == NULL ? 0 : synth->getNumberOfParameters(),
				  dev == NULL ? 0 : dev->getNumberOfSignals() +
				  hmm.getNumHiddenStates())
    {
      if(dev == NULL || PLAYTIME == 0 || synth == NULL || clusters.size() == 0)
	throw std::invalid_argument("ReinforcementSounds ctor: bad arguments");
      
      audio_thread = nullptr;

      this->dev = dev;
      this->hmm = hmm;
      this->clusters = clusters;
      this->rmodel = rmodel;
      this->pictures = pictures;
      this->PLAYTIME = PLAYTIME;
      this->target = target;
      this->targetVar = targetVar;
      this->synth = synth;
      
      this->currentHMMstate = 0;
      
      this->random = false;
      
      this->useReinforcementModel = false;
      
      // starts audio thread NOTE audioLoop assumes
      // ReinforcementPictures() has been started so SDL initializations
      // has been done
      {
	running = true;
	audio_thread = new thread(std::bind(&ReinforcementSounds<T>::audioLoop,
					    this));
      }
    }

    template <typename T>
    ReinforcementSounds<T>::~ReinforcementSounds()
    {
      // stops audio thread
      {
	std::lock_guard<std::mutex> lock(audio_mutex);
	
	running = false;
	audio_thread->join();
      }
    }
    

    template <typename T>
    bool ReinforcementSounds<T>::getSoundIsActive()
    {
      return (this->running);
    }
    
    // if set true use reinforcement model (nnetwork<T>) as
    // reinforcement values..
    template <typename T>
    void ReinforcementSounds<T>::setReinforcementModel(bool useModel)
    {
      useReinforcementModel = useModel;
    }
    
    // sets random mode (if true) instead of showing the best pic,
    // displays randomly chosen pic while doing all the calculations etc.
    template <typename T>
    void ReinforcementSounds<T>::setRandom(bool r)
    {
      random = r;
    }

    template <typename T>
    bool ReinforcementSounds<T>::getState(whiteice::math::vertex<T>& state)
    {
      state.resize(dev->getNumberOfSignals() + hmm.getNumHiddenStates());
      state.zero();
      
      std::vector<float> v;
      
      if(dev->data(v) == false) return false;
      
      for(unsigned int i=0;i<v.size();i++){
	state[i] = v[i];
      }
      
      state[v.size() + currentHMMstate] = T(1.0);
      
      return true;
    }

    template <typename T>
    bool ReinforcementSounds<T>::performAction
    (const whiteice::math::vertex<T>& action,
     whiteice::math::vertex<T>& newstate,
     T& reinforcement)
    {
      
      // adds action to the queue (the next action)
      {
	std::unique_lock<std::mutex> lock(actionMutex);
	actionQueue.push_back(action);
	actionQueue_cond.notify_all();

	// waits for action execution to start
	while(actionQueue.size() > 0 && running){
	  auto now = std::chrono::system_clock::now();
	  actionQueue_cond.wait_until(lock, now + 10*100ms);
	}

	if(running == false)
	  return false;
      }


      usleep(PLAYTIME*1000/2); // we can safely sleep 50% of the play time

      

      // waits for action to be added to old actions queue and clears it
      {
	std::unique_lock<std::mutex> lock(performedActionsMutex);

	while(performedActionsQueue.size() == 0 && running){
	  auto now = std::chrono::system_clock::now();
	  performedActionsQueue_cond.wait_until(lock, now + 10*100ms);
	}

	if(running == false)
	  return false;
      }


      // gets new (response) state
      if(getState(newstate) == false) return false;

      // calculates reinforcement signal ~ ||newstate - target||^2
      {
	reinforcement = T(0.0);
	
	if(useReinforcementModel){
	
	  whiteice::math::vertex<T> m;
	  whiteice::math::matrix<T> c;
	  m.resize(1);
	  m.zero();
	  
	  if(rmodel.calculate(newstate, m, c, 1, 0)){
	    reinforcement = m[0];
	  }
	
	}
	else{
	  
	  for(unsigned int i=0;i<target.size();i++){
	    reinforcement +=
	      (newstate[i].c[0] - target[i])*
	      (newstate[i].c[0] - target[i])/targetVar[i];
	  }
	  
	  // minus: we sought to minimize distance to the target state
	  // (maximize -f(x))
	  reinforcement = -reinforcement; 
	
	}

	// reports average distance to target during latest 150 rounds
	{
	  // T distance = sqrt(reinforcement);
	  T distance = abs(reinforcement);
	  distances.push_back(distance);
	  
	  while(distances.size() > 150)
	    distances.pop_front();
	  
	  if(distances.size() >= 150){
	    
	    T d150 = T(0.0);
	    T dev  = T(0.0);
	    for(const auto& d : distances){
	      d150 += d;
	      dev  += d*d;
	    }
	    
	    d150 /= T(distances.size());
	    dev  /= T(distances.size());
	    dev = sqrt(abs(dev - d150*d150));
	    
	    // reports distance to the target state
	    printf("GOODNESS-150: %f +- %f\n", d150.c[0], dev.c[0]);
	  }
	}

      }
      
      
      return true;
    }

    
    template <typename T>
    void ReinforcementSounds<T>::audioLoop()
    {
      long long start_ms =
	duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
      
      // selects HMM initial state randomly according to starting state probabilities
      auto pi = hmm.getPI();
      currentHMMstate = hmm.sample(pi);

      whiteice::math::vertex<T> action(this->numActions);
      action.zero();
      bool hasAction = false;

      synth->pause();

      // sets silent audio
      {
	std::vector<float> params(synth->getNumberOfParameters());

	for(auto& p : params)
	  p = 0.0f;

	synth->setParameters(params);
      }
      synth->play();

      
      while(running){

	if(dev->connectionOk() == false){
	  running = false;
	  continue;
	}

	// waits for full sound playtime
	{
	  const long long end_ms =
	    duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	  long long delta_ms = end_ms - start_ms;
	  
	  if(delta_ms < PLAYTIME){
	    usleep((PLAYTIME - delta_ms)*1000);
	  }
	}

	// synth->pause();

	// updates HMM hidden state after action
	{
	  std::vector<float> after;
	  std::vector<T> state;
	  
	  after.resize(dev->getNumberOfSignals());
	  for(auto& a : after) a = 0.0f;
	  
	  dev->data(after);
	  
	  state.resize(after.size());
	  
	  for(unsigned int i=0;i<state.size();i++){
	    state[i] = after[i];
	  }
	  
	  const unsigned int o = clusters.getClusterIndex(state);
	  
	  unsigned int nextState = currentHMMstate;
	  
	  const double p = hmm.next_state(currentHMMstate, nextState, o);
	  
	  currentHMMstate = nextState;
	}

	// adds previous action to performedActionsQueue
	if(hasAction){
	  std::lock_guard<std::mutex> lock(performedActionsMutex);
	  performedActionsQueue.push_back(action);
	  performedActionsQueue_cond.notify_all();
	}
	
	// waits for new action
	{
	  std::unique_lock<std::mutex> lock(actionMutex);

	  while(actionQueue.size() == 0 && running){
	    auto now = std::chrono::system_clock::now();
	    actionQueue_cond.wait_until(lock, now + 10*100ms);
	  }

	  if(running == false) continue;
	  
	  action = actionQueue.front();
	  actionQueue.pop_front();
	  actionQueue_cond.notify_all();
	  
	  hasAction = true;
	}
      

	if(random){
	  for(unsigned int i=0;i<action.size();i++)
	    action[i] = rng.uniform();
	}
	
	// starts playing new sound
	std::cout << "!!!!!!!!!!!!!!!!!!!!!! PLAYING SOUND: "
		  << action << std::endl;
	{
	  std::vector<float> params(synth->getNumberOfParameters());
	  
	  for(auto& p : params)
	    p = 0.0f;

	  for(unsigned int i=0;i<params.size()&&i<action.size();i++){
	    params[i] = action[i].c[0];
	    
	    if(params[i] < 0.0f) params[i] = 0.0f;
	    else if(params[i] > 1.0f) params[i] = 1.0f;

	    // params[i] = ((float)rand())/((float)RAND_MAX); // for debugging test
	  }

	  synth->setParameters(params);
	  // synth->play();
	}

	start_ms =
	  duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
      }

      synth->pause(); // pauses sound synthesis when stopped
      
    }
    
    
    
    template class ReinforcementSounds< whiteice::math::blas_real<float> >;
    template class ReinforcementSounds< whiteice::math::blas_real<double> >;
    
  };
};
