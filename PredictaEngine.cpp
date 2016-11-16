
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

      latestError = "No error";
      currentStatus = "Initializing..";

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
	setError("Previous optimization is still running");
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
      setStatus("Waiting..");
      thread_idle = true;

      while(running){
	while(!optimize && running){
	  thread_idle = true;
	  setStatus("Waiting..");
	  usleep(100000); // 100ms (waits for action

	  train.clear();
	  scoring.clear();
	  results.clear();
	}

	if(!running) continue;

	thread_idle = false;

	train.clear();
	scoring.clear();
	results.clear();

	//////////////////////////////////////////////////////////////////////////
	// loads at most 100.000 = 100k lines of data to memory

	setStatus("Loading data (examples)..");

	if(train.importAscii(trainingFile, 100000) == false){
	  std::string error = "Cannot load file: " + trainingFile;
	  setError(error);	  
	  optimize = false;
	  continue;
	}

	setStatus("Loading data (to be scored data)..");

	if(scoring.importAscii(scoringFile, 100000) == false){
	  std::string error = "Cannot load file: " + scoringFile;
	  train.clear();
	  scoring.clear();
	  setError(error);
	  optimize = false;
	  continue;
	}

	setStatus("Checking data validity..");

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
	  continue; // abort computations

	train = tmp;
	
	setStatus("Preprocessing data..");

	if(train.preprocess(0) == false /*|| train.preprocess(1) == false*/){
	  setError("Bad/singular data please add more variance to data");
	  optimize = false;
	  continue;
	}
	
	//////////////////////////////////////////////////////////////////////////
	// optimize neural network using LBFGS (ML starting point for HMC sampler)

	

	
	std::vector<unsigned int> arch; // use double layer wide nnetwork
	arch.push_back(train.dimension(0));
	// arch.push_back(100*train.dimension(0));
	arch.push_back(train.dimension(0) < 10 ? 10 : train.dimension(0));
	arch.push_back(train.dimension(0) < 10 ? 10 : train.dimension(0));
	arch.push_back(train.dimension(1));

	whiteice::nnetwork< whiteice::math::blas_real<double> > nn(arch);
	whiteice::LBFGS_nnetwork< whiteice::math::blas_real<double> > bfgs(nn, train, false, false);
	whiteice::math::vertex< whiteice::math::blas_real<double> > w;

	nn.randomize();

#if 0
	setStatus("Preoptimizing solution (deep learning)..");
	if(deep_pretrain_nnetwork(&nn, train, true) == false){
	  setError("ERROR: deep pretraining of nnetwork failed.\n");
	  optimize = false;
	  continue;
	}
#endif

	
	nn.exportdata(w);
	bfgs.minimize(w);

	time_t t0 = time(0);
	unsigned int iterations = 0;
	whiteice::math::blas_real<double> error;


	while(optimize && bfgs.solutionConverged() == false && bfgs.isRunning() == true){
	  if(optimize == false){ // we lost license to do this anymore..
	    setStatus("Aborting optimization");
	    break;
	  }

	  time_t t1 = time(0);
	  unsigned int counter = (unsigned int)(t1 - t0); // time-elapsed

	  if(bfgs.getSolution(w, error, iterations) == false){ // we lost license to continue..
	    setStatus("Aborting optimization");
	    setError("LBFGS::getSolution() failed");
	    optimize = false;
	    break;
	  }

	  char buffer[128];
	  snprintf(buffer, 128, "Preoptimizing solution (%d iterations, %.2f minutes): %f",
		   iterations, counter/60.0f, error.c[0]);

	  setStatus(buffer);

	  // update results only every 5 seconds
	  sleep(5);
	}

	if(optimize == false){
	  bfgs.stopComputation();
	  continue; // abort computation
	}

	
	// after convergence, get the best solution
	if(bfgs.getSolution(w, error, iterations) == false){ // we lost license to continue..
	  setStatus("Aborting optimization");
	  setError("LBFGS::getSolution() failed");
	  optimize = false;
	  continue;
	}

	nn.importdata(w);

	//////////////////////////////////////////////////////////////////////////
	// use HMC to sample from max likelihood in order to get MAP

	setStatus("Analyzing uncertainty..");

	// whiteice::UHMC< whiteice::math::blas_real<double> > hmc(nn, train, true);
	whiteice::HMC< whiteice::math::blas_real<double> > hmc(nn, train, true);
	// whiteice::linear_ETA<float> eta;

	// for high quality..
	// we use just 50 samples
	// const unsigned int NUMSAMPLES = 1000;
	// eta.start(0.0f, NUMSAMPLES);

	if(hmc.startSampler() == false){
	  setStatus("Starting sampler failed (internal error)");
	  setError("Cannot start sampler");
	  optimize = false;
	  continue;
	}

	// unsigned int samples = 0;

	t0 = time(0);
	
	// always analyzes results for 15 minutes
	unsigned int totalTime = 15*60; 
	

	while(optimize){
	  unsigned int samples = hmc.getNumberOfSamples();
	  // if(samples >= NUMSAMPLES) break;
	  
	  // eta.update((float)hmc.getNumberOfSamples());

	  time_t t1 = time(0);
	  unsigned int counter =
	    (unsigned int)(t1 - t0); // time-elapsed

	  double timeLeft = (totalTime - counter)/60.0;
	  if(timeLeft <= 0.0){
	    timeLeft = 0.0;
	    break;
	  }

	  char buffer[128];
	  snprintf(buffer, 128,
		   "Analyzing uncertainty (%d iterations. %.2f%% error. ETA %.2f minutes)",
		   // 100.0*((double)samples/((double)NUMSAMPLES)),
		   samples,
		   100.0*hmc.getMeanError(1).c[0]/error.c[0],
		   timeLeft);
		   // eta.estimate()/60.0);

	  setStatus(buffer);

	  if(optimize == false){ // we lost license to continue..
	    setStatus("Uncertainty analysis aborted");
	    break;
	  }

	  // updates only every 5 seconds so that we do not take too much resources
	  sleep(5); 
	}

	if(optimize == false)
	  continue; // abort computation

	hmc.stopSampler();
	
	//////////////////////////////////////////////////////////////////////////
	// estimate mean and variance of output given inputs in 'scoring'

	setStatus("Calculating scoring..");

	whiteice::bayesian_nnetwork< whiteice::math::blas_real<double> > bnn;
	
	if(hmc.getNetwork(bnn) == false){
	  setStatus("Exporting prediction model failed");
	  setError("Internal software error");
	  optimize = false;
	  continue;
	}
	
	if(results.createCluster("results", 1) == false){
	  setError("Internal software error");
	  optimize = false;
	  continue;
	}



	for(unsigned int i=0;i<scoring.size(0);i++){

	  char buffer[128];
	  snprintf(buffer, 128, "Scoring data (%.1f%%)..",
		   100.0*((double)i)/((double)scoring.size(0)));
	  setStatus(buffer);
	  
	  
	  whiteice::math::vertex< whiteice::math::blas_real<double> > mean;
	  whiteice::math::matrix< whiteice::math::blas_real<double> > cov;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > score;
	  auto tmp = scoring[i];

	  if(train.preprocess(0, tmp) == false){
	    setStatus("Calculating prediction failed (preprocess)");
	    setError("Internal software error");
	    optimize = false;
	    break;
	  }
	  
	  if(bnn.calculate(tmp, mean, cov) == false){
	    setStatus("Calculating prediction failed");
	    setError("Internal software error");
	    optimize = false;
	    break;
	  }

	  score.resize(1);
	  score[0] = mean[0] + risk*cov(0,0);

	  if(results.add(0, score) == false){
	    setStatus("Calculating prediction failed (storage)");
	    setError("Internal software error");
	    optimize = false;
	    break;
	  }

	  if(optimize == false)
	    break; // lost our license to continue
	}

	if(optimize == false)
	  continue; // lost our license to continue


	// finally save the results
	setStatus("Saving prediction results to file..");

	if(results.exportAscii(resultsFile) == false){
	  setStatus("Saving prediction results failed");
	  setError("Internal software error");
	  optimize = false;
	  break;
	}

	setStatus("Computations complete");
	
	optimize = false;
      }
    }
    
  };
};
