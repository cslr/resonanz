
#include "PredictaEngine.h"
#include <stdio.h>
#include <unistd.h>

#include <dinrhiw/dinrhiw.h>

#include <vector>


namespace whiteice
{
  namespace resonanz
  {
    
    PredictaEngine::PredictaEngine()
    {
      running = false;
      thread_idle = true;
      optimize = false;
      worker_thread = nullptr;

      latestError = "no error";
      currentStatus = "initializing..";

      try{
	running = true;
	worker_thread = new std::thread(&PredictaEngine::loop, this);
      }
      catch(std::exception){
	running = false;
	throw std::runtime_error("PredictaEngine: couldn't create worker thread.");
      }
	
    }

    
    PredictaEngine::~PredictaEngine()
    {
      running = false;
      optimize = false;
      if(worker_thread != nullptr)
	worker_thread->join();
    }

    
    bool PredictaEngine::startOptimization(const std::string& trainingFile,
					   const std::string& scoringFile,
					   const std::string& resultsFile,
					   double risk)
    {
      if(access(trainingFile.c_str(), R_OK) != 0){
	setError("Cannot read training file");
	return false;
      }

      if(access(scoringFile.c_str(), R_OK) != 0){
	setError("Cannot read scoring file");
	return false;
      }

      if(isnan(risk) || isinf(risk)){
	setError("Risk is not finite number");
	return false;
      }

      std::lock_guard<std::mutex> lock(optimizeLock);

      if(optimize) return false; // already optimizing (only single process per object)

      if(thread_idle == false){
	setError("previous optimization is still running");
	return false; // thread is still running
      }

      this->trainingFile = trainingFile;
      this->scoringFile  = scoringFile;
      this->resultsFile  = resultsFile;
      this->risk = risk;

      optimize = true;

      return true;
    }

    
    bool PredictaEngine::isActive()
    {      
      return (optimize && running);
    }

    
    bool PredictaEngine::stopOptimization()
    {
      std::lock_guard<std::mutex> lock(optimizeLock);
      
      if(optimize){
	optimize = false;
	
	return true;
      }
      else return false;
    }

    
    std::string PredictaEngine::getError()
    {
      std::lock_guard<std::mutex> lock(error_mutex);
      return latestError;
    }

    
    std::string PredictaEngine::getStatus()
    {
      std::lock_guard<std::mutex> lock(status_mutex);
      return currentStatus;
    }

    
    void PredictaEngine::setStatus(const std::string& status){
      std::lock_guard<std::mutex> lock(status_mutex);
      currentStatus = status;
    }
    
    void PredictaEngine::setError(const std::string& error){
      std::lock_guard<std::mutex> lock(error_mutex);
      latestError = error;
    }

    
    //////////////////////////////////////////////////////////////////////
    // worker thread
    void PredictaEngine::loop()
    {
      setStatus("waiting..");
      thread_idle = true;

      while(running){
	while(!optimize && running){
	  thread_idle = true;
	  setStatus("waiting..");
	  usleep(100000); // 100ms (waits for action
	}

	if(!running) continue;

	thread_idle = false;

	train.clear();
	scoring.clear();
	results.clear();

	printf("P1\n"); fflush(stdout);
	
	//////////////////////////////////////////////////////////////////////////
	// attempt to load all data or 1.000.000 (million) lines of data to memory

	setStatus("loading data (examples)..");

	if(train.importAscii(trainingFile, 1000000) == false){
	  std::string error = "Cannot load file: " + trainingFile;
	  setError(error);	  
	  optimize = false;
	  continue;
	}

	setStatus("loading data (to be scored data)..");

	if(scoring.importAscii(scoringFile, 1000000) == false){
	  std::string error = "Cannot load file: " + scoringFile;
	  train.clear();
	  scoring.clear();
	  setError(error);
	  optimize = false;
	  continue;
	}

	printf("P2\n"); fflush(stdout);

	setStatus("checking data validity..");

	train.removeBadData();
	scoring.removeBadData();
	
	// if number of data points in training is smaller than 2*dim(input)
	// then the optimizer fails
	
	if(train.getNumberOfClusters() < 0 || scoring.getNumberOfClusters() < 0){
	  setError("No data in input files");
	  optimize = false;
	  continue;
	}
	
	if(train.size(0) < 10){
	  setError("Not enough data (at least 10 examples) in input file");
	  optimize = false;
	  continue;
	}
	
	if(train.size(0) < 2*train.dimension(0)){
	  setError("Not enough data (at least 2*DIMENSION examples) in input file");
	  optimize = false;
	  continue;
	}

	if(train.dimension(0) != (scoring.dimension(0) + 1)){
	  setError("Incorrect dimensions in training or scoring files");
	  optimize = false;
	  continue;
	}

	if(train.dimension(0) < 2){
	  setError("Incorrect dimensions in training or scoring files");
	  optimize = false;
	  continue;
	}

	printf("P3\n"); fflush(stdout);
	
	//////////////////////////////////////////////////////////////////////////
	// preprocess data using PCA (if PCA cannot be calculated the whole process fails)

	// separates training data into input and output clusters
	whiteice::dataset< whiteice::math::blas_real<double> > tmp;

	if(tmp.createCluster("input", train.dimension(0)-1) == false || 
	   tmp.createCluster("output", 1) == false){
	  setError("Internal software error");
	  optimize = false;
	  continue;
	}

	for(unsigned int i=0;i<train.size(0);i++){
	  whiteice::math::vertex< whiteice::math::blas_real<double> > t = train.access(0, i);
	  whiteice::math::vertex< whiteice::math::blas_real<double> > a;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > b;
	  a.resize(t.size()-1);
	  b.resize(1);
	  b[0] = t[t.size()-1];

	  for(unsigned int j=0;j<(t.size()-1);j++)
	    a[j] = t[j];

	  if(tmp.add(0, a) == false || tmp.add(1, b) == false){
	    setError("Internal software error");
	    optimize = false;
	    continue;
	  }
	}

	if(optimize == false)
	  continue; // error

	printf("P4\n"); fflush(stdout);

	train = tmp;
	
	setStatus("preprocessing (PCA) data..");

	if(train.preprocess(0) == false || train.preprocess(1) == false){
	  setError("Bad/singular data please add more variance to data");
	  optimize = false;
	  continue;
	}
	
	//////////////////////////////////////////////////////////////////////////
	// optimize neural network using LBFGS (ML starting point for HMC sampler)

	printf("P5\n"); fflush(stdout);

	setStatus("preoptimizing solution..");

	std::vector<unsigned int> arch; // use double layer wide nnetwork
	arch.push_back(train.dimension(0));
	arch.push_back(10*train.dimension(0));
	arch.push_back(10*train.dimension(0));
	arch.push_back(train.dimension(1));

	whiteice::nnetwork< whiteice::math::blas_real<double> > nn(arch);
	whiteice::LBFGS_nnetwork< whiteice::math::blas_real<double> > bfgs(nn, train, false, false);
	whiteice::math::vertex< whiteice::math::blas_real<double> > w;

	nn.randomize();
	nn.exportdata(w);
	bfgs.minimize(w);

	time_t t0 = time(0);
	unsigned int iterations = 0;
	whiteice::math::blas_real<double> error;


	while(bfgs.solutionConverged() == false && bfgs.isRunning() == true){
	  if(optimize == false){ // we lost license to do this anymore..
	    setStatus("aborting optimization");
	    continue;
	  }

	  time_t t1 = time(0);
	  unsigned int counter = (unsigned int)(t1 - t0); // time-elapsed

	  if(bfgs.getSolution(w, error, iterations) == false){ // we lost license to continue..
	    setStatus("aborting optimization");
	    setError("LBFGS::getSolution() failed");
	    optimize = false;
	    continue;
	  }

	  char buffer[80];
	  snprintf(buffer, 80, "preoptimizing solution (%d iterations, %f minutes): %f",
		   iterations, counter/60.0f, error.c[0]);

	  setStatus(buffer);
	  
	  sleep(1);
	}

	// after convergence, get the best solution
	if(bfgs.getSolution(w, error, iterations) == false){ // we lost license to continue..
	  setStatus("aborting optimization");
	  setError("LBFGS::getSolution() failed");
	  optimize = false;
	  continue;
	}

	if(optimize == false)
	  continue; // abort computation

	nn.importdata(w);

	//////////////////////////////////////////////////////////////////////////
	// use HMC to sample from max likelihood in order to get MAP

	setStatus("Analyzing uncertainty..");

	printf("P6\n"); fflush(stdout);

	whiteice::UHMC< whiteice::math::blas_real<double> > hmc(nn, train, true);
	whiteice::linear_ETA<float> eta;

	// for high quality..
	// we get 10.000 samples and throw away the first 5.000 samples
	const unsigned int NUMSAMPLES = 10000;
	eta.start(0.0f, NUMSAMPLES);

	hmc.startSampler();

	unsigned int samples = 0;

	while(optimize){
	  samples = hmc.getNumberOfSamples();
	  if(samples >= NUMSAMPLES) break;
	  
	  eta.update((float)hmc.getNumberOfSamples());

	  char buffer[128];
	  snprintf(buffer, 128, "Analyzing uncertainty (%d samples. %f error. ETA %f minutes)",
		   samples, hmc.getMeanError(25).c[0], eta.estimate()/60.0);

	  setStatus(buffer);

	  if(optimize == false){ // we lost license to live..
	    setStatus("Uncertainty analysis aborted");
	    optimize = false;
	    break;
	  }

	  sleep(1);
	}

	printf("P7\n"); fflush(stdout);
	
	//////////////////////////////////////////////////////////////////////////
	// estimate mean and variance of output given inputs in 'scoring'

	// TODO
	
      }
    }
    
  };
};
