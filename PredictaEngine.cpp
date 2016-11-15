
#include "PredictaEngine.h"
#include <stdio.h>
#include <unistd.h>



namespace whiteice
{
  namespace resonanz
  {
    
    PredictaEngine::PredictaEngine()
    {
      running = false;
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
      if(worker_thread != nullptr)
	worker_thread->join();
    }

    bool PredictaEngine::startOptimization(const std::string& traininigFile,
					   const std::string& scoringFile,
					   const std::string& resultsFile,
					   double risk)
    {
      if(access(trainingFile.c_str(), R_OK) != 0 ||
	 access(scoringFile.c_str(), R_OK) != 0 ||
	 access(resultsFile.c_str(), W_OK) != 0)
	return false;

      if(isnan(risk) || isinf(risk)) return false;

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
      if(optimize){
	optimize = false;
	return true;
      }
      else return false;
    }
    
    std::string PredictaEngine::getError()
    {
      return latestError;
    }
    
    std::string PredictaEngine::getStatus()
    {
      return currentStatus;
    }

    // worker thread
    void PredictaEngine::loop()
    {
      setStatus("waiting..");

      while(running){
	while(!optimize && running){
	  setStatus("waiting..");
	  usleep(100000); // 100ms (waits for action
	}

	if(!running) continue;

	train.clear();
	scoring.clear();
	results.clear();
	
	//////////////////////////////////////////////////////////////////////////
	// attempt to load all data or 1.000.000 (million) lines of data to memory

	setStatus("loading data (examples)..");

	if(train.importAscii(0, trainingFile, 1000000) == false){
	  std::string error = "Cannot load file: " + trainingFile;
	  setError(error);
	  optimize = false;
	  continue;
	}

	setStatus("loading data (to be scored data)..");

	if(scoring.importAscii(0, scoringFile, 1000000) == false){
	  std::string error = "Cannot load file: " + scoringFile;
	  train.clear();
	  scoring.clear();
	  setError(error);
	  optimize = false;
	  continue;
	}

	setStatus("checking data validity..");

	train.removeBadData();
	scoring.removeBadData();
	
	// if number of data points in training is smaller than 2*dim(input)
	// then the optimizer fails
	
	if(train.size() < 0 || scoring.size() < 0){
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

	  if(tmp.add(0, a) == false || tmp.add(1, a) == false){
	    setError("Internal software error");
	    optimize = false;
	    continue;
	  }
	}

	train = tmp;
	
	setStatus("preprocessing (PCA) data..");

	if(train.preprocess(0) == false || train.preprocess(1)){
	  setError("Bad/singular data please add more variance to data");
	  optimize = false;
	  continue;
	}

	//////////////////////////////////////////////////////////////////////////
	// optimize neural network using LBFGS (ML starting point for HMC sampler)

	//////////////////////////////////////////////////////////////////////////
	// use HMC to sample from max likelihood in order to get MAP

	//////////////////////////////////////////////////////////////////////////
	// estimate mean and variance of output given inputs in 'scoring'
	
	
      }
    }
    
  };
};
