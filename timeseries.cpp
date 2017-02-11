/*
 * Time Series Analysis of EEG using HMM
 *
 * (C) Copyright Tomas Ukkonen 2017
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <dinrhiw/dinrhiw.h>

#include <vector>
#include <string>

#include "DataSource.h"
#include "RandomEEG.h"
#include "MuseOSC.h"

#include "ts_measure.h"
#include "ReinforcementPictures.h"

void print_usage();

bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& device,
		      std::string& dir,
		      std::vector<double>& targets, 
		      std::vector<double>& targetVar,
		      std::vector<std::string>& pictures);


#define DISPLAYTIME 250 // picture display time in msecs


using namespace whiteice::resonanz;


int main(int argc, char** argv)
{
  printf("Time Series Analysis (HMM) 0.01 (C) Copyright Tomas Ukkonen\n");

  srand(time(0));
  
  std::string cmd;
  std::string device;
  std::string dir;
  std::vector<std::string> pictures;
  std::vector<double> targetVector;
  std::vector<double> targetVar;

  // Hidden Markov Model parameters
  // assumes 100 different clusterized discrete brain EEG states
  const unsigned int VISIBLE_SYMBOLS = 100; 
  const unsigned int HIDDEN_STATES   = 5;   // HMM hidden states



  if(!parse_parameters(argc, argv, cmd, device, dir, targetVector, targetVar, pictures)){
    print_usage();
    return -1;
  }

  // shows random pictures instead of ones computed to be most effective
  bool random = false;

  // uses nnetwork as state to reinforcement model instead of distance to target
  bool useRmodel = false;
  
  if(cmd == "--stimulater"){
    cmd = "--stimulate";
    random = true;
  }

  if(cmd == "--reinforcementr"){
    cmd = "--reinforcement";
    random = true;
  }

  // model filenames
  char buffer[256];
  
  snprintf(buffer, 256, "%s/%s-measurements.dat", dir.c_str(), device.c_str());
  const std::string timeseriesFile = buffer;
  
  snprintf(buffer, 256, "%s/%s-hmm.model", dir.c_str(), device.c_str());
  const std::string hmmFile     = buffer;
  
  snprintf(buffer, 256, "%s/%s-clusters.model", dir.c_str(), device.c_str());
  const std::string clusterModelFile = buffer;

  snprintf(buffer, 256, "%s/%s-state-measurements.dat", dir.c_str(), device.c_str());
  const std::string stateDataFile = buffer;

  snprintf(buffer, 256, "%s/%s-state.model", dir.c_str(), device.c_str());
  const std::string stateModelFile = buffer;

  snprintf(buffer, 256, "%s/%s-reinforcement.model", dir.c_str(), device.c_str());
  const std::string reinforcementModelFile = buffer;

  //////////////////////////////////////////////////////////////////////

  if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
    printf("ERROR: SDL initialization failed: %s\n", SDL_GetError());
    return -1;
  }

  const unsigned int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    
  if(IMG_Init(img_flags) != img_flags){
    printf("ERROR: SDL IMAGE initialization failed\n");
    return -1;
  }

  if(TTF_Init() != 0){
    printf("Truetype font system initialization failed\n");
    return -1;
  }
  
  //////////////////////////////////////////////////////////////////////

  if(cmd == "--measure1"){
    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(1);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    whiteice::dataset< whiteice::math::blas_real<double> > timeseries;

    timeseries.load(timeseriesFile); // attempts to load previously measured time-series
    
    if(measureRandomPicturesAndTimeSeries(dev, pictures,
					  DISPLAYTIME, timeseries) == false)
    {
      printf("ERROR: measuring time series failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    if(timeseries.save(timeseriesFile) == false){
      printf("ERROR: saving time series measurements to file failed\n");
      
      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    printf("Saving %d step length time series observation..\n", timeseries.size(0));

    delete dev;
  }
  else if(cmd == "--learn1"){

    whiteice::dataset< whiteice::math::blas_real<double> > timeseries;

    if(timeseries.load(timeseriesFile) == false){
      printf("ERROR: loading time series measurements from file failed\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    std::vector<unsigned int> observations;
    whiteice::KMeans< whiteice::math::blas_real<double> > clusters;

    // calculates discretization 
    {
      std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > data;
      if(timeseries.getData(0, data) == false){
	printf("ERROR: bad time series data\n");
	IMG_Quit();
	SDL_Quit();

	delete dev;

	return -1;
      }

      printf("Calculating K-Means clusters (timeseries data)..\n");

      if(clusters.learn(VISIBLE_SYMBOLS, data) == false){
	printf("ERROR: calculating k-means clusters failed\n");
      }

      for(unsigned int i=0;i<data.size();i++){
	observations.push_back(clusters.getClusterIndex(data[i]));
      }

      if(clusters.save(clusterModelFile) == false){
	printf("ERROR: saving clusters model failed.\n");

	delete dev;
	IMG_Quit();
	SDL_Quit();
	
	return -1;
      }
      
    }

    // learns from data
    try{
      double logp = hmm.train(observations);

      printf("DATA LIKELIHOOD: %f\n", logp);
      
    }
    catch(std::invalid_argument& e){
      printf("ERROR: learning from HMM data failed: %s\n", e.what());

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    if(hmm.save(hmmFile) == false){
      printf("ERROR: saving HMM model failed.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    else{
      printf("Saving HMM model file successful.\n");
    }

    return 0;
  }
  else if(cmd == "--predict"){

    whiteice::KMeans< whiteice::math::blas_real<double> > clusters;

    if(clusters.load(clusterModelFile) == false){
      printf("ERROR: loading measurements cluster model file failed.\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(1);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }


    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    if(hmm.load(hmmFile) == false){
      printf("ERROR: loading Hidden Markov Model (HMM) failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    if(predictHiddenState(dev, hmm, pictures,
			  DISPLAYTIME, clusters) == false)
    {
      printf("ERROR: measuring time series failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
    return 0;
  }
  else if(cmd == "--record-state"){
    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(1);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }


    whiteice::dataset< whiteice::math::blas_real<double> > stateMeasurements;

    if(stateMeasurements.load(stateDataFile)){
      if(stateMeasurements.getNumberOfClusters() == 2)
	printf("Loaded %d old response recordings..\n", stateMeasurements.size(1));
    }

    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    if(hmm.load(hmmFile) == false){
      printf("ERROR: loading Hidden Markov Model (HMM) failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
    whiteice::KMeans< whiteice::math::blas_real<double> > clusters;

    if(clusters.load(clusterModelFile) == false){
      printf("ERROR: loading measurements cluster model file failed.\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
    // measures user reported responses to pictures
    if(measureResponsePictures(dev, hmm, pictures, DISPLAYTIME,
			       clusters,
			       stateMeasurements) == false)
    {
      printf("ERROR: measuring responses failed\n");
    }

    if(stateMeasurements.getNumberOfClusters() == 2){
      if(stateMeasurements.save(stateDataFile)){
	printf("Saved %d responses to pictures..\n", stateMeasurements.size(1));
      }
    }
    
  }
  else if(cmd == "--learn-state"){

    whiteice::dataset< whiteice::math::blas_real<double> > stateMeasurements;
    bool loadOK = false;

    if(stateMeasurements.load(stateDataFile)){
      if(stateMeasurements.getNumberOfClusters() == 2){
	printf("Loaded %d response recordings..\n", stateMeasurements.size(1));
	loadOK = true;
      }
    }

    if(loadOK == false){
      printf("Loading response recordings file FAILED.\n");

      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    
    whiteice::nnetwork< whiteice::math::blas_real<double> > net;
    double error = 0.0;

    printf("Optimizing response model..\n");
    fflush(stdout);

    if(optimizeNeuralnetworkModel(stateMeasurements, net, error) == false){
      printf("Optimizing neural network response model FAILED\n");
	
      IMG_Quit();
      SDL_Quit();
	
      return -1;
    }

    printf("State response model error: %f\n", error);
    fflush(stdout);

    whiteice::bayesian_nnetwork< whiteice::math::blas_real<double> > bnet;
    bnet.importNetwork(net);

    if(bnet.save(stateModelFile) == false){
      printf("ERROR: saving state response model failed\n");
      
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }
    
  }
  else if(cmd == "--measure2"){
    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(1);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    std::vector< whiteice::dataset< whiteice::math::blas_real<double> > > picmeasurements(pictures.size());

    {
      for(unsigned int i=0;i<pictures.size();i++){
	sprintf(buffer, "%s.%s.dat", pictures[i].c_str(), device.c_str());
	std::string file = buffer;
	picmeasurements[i].load(file); // attempts to load the previous measurements
      }
    }

    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    if(hmm.load(hmmFile) == false){
      printf("ERROR: loading Hidden Markov Model (HMM) failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
    whiteice::KMeans< whiteice::math::blas_real<double> > clusters;

    if(clusters.load(clusterModelFile) == false){
      printf("ERROR: loading measurements cluster model file failed.\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
    
    if(measureRandomPictures(dev, hmm, pictures, DISPLAYTIME,
			     clusters,
			     picmeasurements) == false)
    {
      printf("ERROR: measuring picture-wise responses failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    unsigned int maxsize = 0;
    unsigned int minsize = picmeasurements[0].size(0);
    unsigned int measurements = 0;

    {
      for(unsigned int i=0;i<pictures.size();i++){
	sprintf(buffer, "%s.%s.dat", pictures[i].c_str(), device.c_str());
	std::string file = buffer;
	if(picmeasurements[i].save(file) == false){ // attempts to save measurements
	  printf("ERROR: saving picture-wise (%d) measurements FAILED\n", i);
	  delete dev;
	  IMG_Quit();
	  SDL_Quit();
	  
	  return -1;
	}

	if(picmeasurements[i].getNumberOfClusters() > 0){
	  if(picmeasurements[i].size(0) > maxsize)
	    maxsize = picmeasurements[i].size(0);
	  if(picmeasurements[i].size(0) < minsize)
	    minsize = picmeasurements[i].size(0);

	  measurements += picmeasurements[i].size(0);
	}
	else{
	  printf("Zero measurements: %d %s\n", i, pictures[i].c_str());
	  fflush(stdout);
	  minsize = 0;
	}
      }

      printf("Picture (%d pics) measurements (total %d) saved to disk (max %d, min %d measurements per pic)\n",
	     pictures.size(), measurements, maxsize, minsize);
      fflush(stdout);
    }

    delete dev;
    
  }
  else if(cmd == "--list"){

    std::vector< whiteice::dataset< whiteice::math::blas_real<double> > > picmeasurements(pictures.size());

    {
      for(unsigned int i=0;i<pictures.size();i++){
	sprintf(buffer, "%s.%s.dat", pictures[i].c_str(), device.c_str());
	std::string file = buffer;
	picmeasurements[i].load(file); // attempts to load the previous measurements
      }
    }

    // optimizes picture-wise neural network
    for(unsigned int i=0;i<pictures.size();i++){
      whiteice::dataset< whiteice::math::blas_real<double> >& data = picmeasurements[i];
      
      for(unsigned int s=0;s<data.size(1);s++){
	std::cout << data.access(1, s) << std::endl;
      }
      
    }

    
  }
  else if(cmd == "--learn2"){

    std::vector< whiteice::dataset< whiteice::math::blas_real<double> > > picmeasurements(pictures.size());

    {
      for(unsigned int i=0;i<pictures.size();i++){
	sprintf(buffer, "%s.%s.dat", pictures[i].c_str(), device.c_str());
	std::string file = buffer;
	picmeasurements[i].load(file); // attempts to load the previous measurements
      }
    }

    std::vector<double> errors;

    whiteice::linear_ETA<double> eta;

    eta.start(0.0, (double)pictures.size());

    // optimizes picture-wise neural network
    for(unsigned int i=0;i<pictures.size();i++){
      sprintf(buffer, "%s.%s.model", pictures[i].c_str(), device.c_str());
      std::string file = buffer;

      whiteice::nnetwork< whiteice::math::blas_real<double> > net;
      double error = 0.0;

      printf("Optimizing neural network model %d/%d [ETA %.2f minutes]\n", i+1, pictures.size(), eta.estimate()/60.0);
      fflush(stdout);

      if(optimizeNeuralnetworkModel(picmeasurements[i], net, error) == false){
	printf("ERROR: optimizing neural network prediction model %d/%d failed\n", i+1, pictures.size());
	
	IMG_Quit();
	SDL_Quit();
	
	return -1;
      }

      whiteice::bayesian_nnetwork< whiteice::math::blas_real<double> > bnet;
      bnet.importNetwork(net);

      if(bnet.save(file) == false){
	printf("ERROR: saving neural network prediction model %d/%d failed\n", i+1, pictures.size());
	
	IMG_Quit();
	SDL_Quit();
	
	return -1;
      }

      eta.update((double)(1.0+i));

      errors.push_back(error);
    }

    // calculates statistics of errors
    {
      double me = 0.0, se = 0.0;

      for(auto& e : errors){
	me += e;
	se += e*e;
      }

      me /= ((double)errors.size());
      se /= ((double)errors.size());

      se = sqrt(fabs(se - me*me));

      printf("MEAN ERROR: %e +- (st.dev) %e\n", me, se);
      fflush(stdout);
    }
    
    
  }
  else if(cmd == "--stimulate"){

    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(1);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    if(targetVector.size() != targetVar.size() || targetVector.size() != dev->getNumberOfSignals()){
      printf("ERROR: No proper target vector for stimulation\n");
      
      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    std::vector< whiteice::dataset< whiteice::math::blas_real<double> > > picmeasurements(pictures.size());

    {
      for(unsigned int i=0;i<pictures.size();i++){
	sprintf(buffer, "%s.%s.dat", pictures[i].c_str(), device.c_str());
	std::string file = buffer;
	picmeasurements[i].load(file); // attempts to load the previous measurements
      }
    }

    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    if(hmm.load(hmmFile) == false){
      printf("ERROR: loading Hidden Markov Model (HMM) failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    whiteice::KMeans< whiteice::math::blas_real<double> > clusters;

    if(clusters.load(clusterModelFile) == false){
      printf("ERROR: loading measurements cluster model file failed.\n");
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    

    std::vector< whiteice::bayesian_nnetwork< whiteice::math::blas_real<double> > > nets;
    nets.resize(pictures.size());

    for(unsigned int i=0;i<pictures.size();i++){
      sprintf(buffer, "%s.%s.model", pictures[i].c_str(), device.c_str());
      std::string file = buffer;

      if(nets[i].load(file) == false){
	printf("ERROR: loading picture response model (neural network) %d/%d from file failed\n",
	       i+1, pictures.size());

	delete dev;
	IMG_Quit();
	SDL_Quit();
	
	return -1;
      }
    }
    
    if(stimulateUsingModel(dev, hmm, nets, pictures, DISPLAYTIME,
			   clusters, targetVector, targetVar, random) == false)
    {
      printf("ERROR: stimulating cns using pictures failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }
    
  }
  else if(cmd == "--device-values"){
    DataSource* dev = nullptr;

    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(2);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    while(dev->connectionOk()){
      std::vector<float> values;
      std::vector<std::string> signals;

      if(dev->data(values) && dev->getSignalNames(signals)){
	for(unsigned int i=0;i<values.size();i++){
	  printf("%.2f ", values[i]);
	}
	printf("\n");
	
	// printf("%s %f\n", signals[2].c_str(), values[2]);
	
	fflush(stdout);
      }

      sleep(1);
    }

    delete dev;
  }
  else if(cmd == "--reinforcement"){
    
    DataSource* dev = nullptr;
    
    if(device == "muse") dev = new whiteice::resonanz::MuseOSC(4545);
    else if(device == "random") dev = new whiteice::resonanz::RandomEEG();

    sleep(1);

    if(dev->connectionOk() == false)
    {
      printf("ERROR: No connection to device.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    if(targetVector.size() != targetVar.size() || targetVector.size() != dev->getNumberOfSignals()){
      printf("ERROR: No proper target vector for stimulation.\n");
      
      delete dev;
      IMG_Quit();
      SDL_Quit();
      
      return -1;
    }

    
    whiteice::HMM hmm(VISIBLE_SYMBOLS, HIDDEN_STATES);

    if(hmm.load(hmmFile) == false){
      printf("ERROR: loading Hidden Markov Model (HMM) failed\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    whiteice::KMeans< whiteice::math::blas_real<double> > clusters;

    if(clusters.load(clusterModelFile) == false){
      printf("ERROR: loading measurements cluster model file failed.\n");

      delete dev;
      IMG_Quit();
      SDL_Quit();

      return -1;
    }

    // reinforcement model r(state) -> reinforcement

    whiteice::bayesian_nnetwork< whiteice::math::blas_real<double> > rmodel;

    if(useRmodel){
      if(rmodel.load(stateModelFile) == false){
	printf("ERROR: loading reinforcement model file failed.\n");
	
	delete dev;
	IMG_Quit();
	SDL_Quit();
	
	return -1;
      }
    }

    
    printf("Starting reinforcement learning..\n");
    fflush(stdout);

    // reinforcement learning
    {
      whiteice::ReinforcementPictures< whiteice::math::blas_real<double> >
	system(dev, hmm, clusters, rmodel, pictures, DISPLAYTIME,
	       targetVector, targetVar);

      if(useRmodel)
	system.setReinforcementModel(true);
      else
	system.setReinforcementModel(false);

      system.setRandom(random);

      system.setLearningMode(true);
      system.setEpsilon(0.60);
      
      system.start();
      unsigned int counter = 0;

      while(system.isRunning()){
	sleep(1);
	if(system.getKeypress() || system.getDisplayIsRunning() == false){
	  system.stop();
	}

	if(counter >= 180){
	  system.save(reinforcementModelFile);
	  counter = 0; // saves model data every 3 minutes
	}

	counter++;
      }
      
    }

  }
  

  
  
  return 0;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


void print_usage()
{
  printf("Usage: timeseries <cmd> <device> <directory> [--target=0.0,0.0,0.0,0.0,0.0,0.0] [--target-var=1.0,1.0,1.0,1.0,1.0,1.0\n");
  printf("       <cmd> = \"--measure1\"       stimulates cns randomly and collects time-series measurements\n");
  printf("       <cmd> = \"--learn1\"         optimizes HMM model using measurements and optimizes neural network models using input and additional hidden states\n");
  printf("       <cmd> = \"--record-state\"   shows stimuli and collects use specified inputs in range of 1-10 about how high user is in the target state\n");
  printf("       <cmd> = \"--learn-state\"    calculates model file for user response state\n");
  printf("       <cmd> = \"--predict\"        predicts and outputs currently predicted hidden state\n");
  printf("       <cmd> = \"--measure2\"       stimulates cns randomly and collects picture-wise information (incl. hidden states)\n");
  printf("       <cmd> = \"--learn2\"         optimizes neural network model per picture using input and additional hidden states\n");
  printf("       <cmd> = \"--stimulate\"      uses model, pictures in directory and neural network models to push brain towards target state\n");
  printf("       <cmd> = \"--stimulater\"     stimulates cns using random pictures [for comparing effectiveness]\n");
  printf("       <cmd> = \"--list\"           lists collected data (output cluster)\n");
  printf("       <cmd> = \"--reinforcement\"  reinforcement learning (requires --measure1 and --learn1)\n");
  printf("       <cmd> = \"--reinforcementr\" reinforcement learning: random mode [for compaing effectiveness] (requires --measure1 and --learn1)\n");
  printf("       <cmd> = \"--device-values\"  prints device values (delta, theta, alpha, beta, gamma, total)\n");
  printf("       <device>                   'muse' (interaxon muse osc.udp://localhost:4545) or 'random' pseudorandom measurements\n");
  printf("       <directory>                path to directory (png and model files)\n");
  printf("       --target=<vector>          optimization target [0,1]^6 vector. (?) value means value can be anything\n");
  printf("                                  <vector>= delta, theta, alpha, beta, gamma, total power scaled within [0,1]\n");
  printf("\n");
}


bool parse_parameters(int argc, char** argv,
		      std::string& cmd,
		      std::string& device,
		      std::string& dir,
		      std::vector<double>& targets, // negative values mean given target value is ignored
		      std::vector<double>& targetVar,
		      std::vector<std::string>& pictures)
{
  if(argc != 4 && argc != 6)
    return false;

  cmd = argv[1];
  device = argv[2];
  dir = argv[3];
  pictures.clear();
  targets.clear();
  targetVar.clear();

  if(cmd != "--measure1" &&
     cmd != "--learn1" &&
     cmd != "--record-state" &&
     cmd != "--learn-state" && 
     cmd != "--predict" &&
     cmd != "--measure2" &&
     cmd != "--learn2" && 
     cmd != "--stimulate" && 
     cmd != "--list" &&
     cmd != "--stimulater" &&
     cmd != "--device-values" && 
     cmd != "--reinforcement" && 
     cmd != "--reinforcementr")
    return false;

  if(device != "muse" &&
     device != "random")
    return false;

  if((argc != 6) && (cmd == "--stimulate" || cmd == "--stimulater" || cmd == "--reinforcement" || cmd == "--reinforcementr"))
    return false;

  if(cmd == "--stimulate" || cmd == "--stimulater" || cmd == "--reinforcement" || cmd == "--reinforcementr"){
    if(strncmp(argv[4], "--target=", 9) == 0){
      char* v = argv[4] + 9;
      char* ptr = NULL;
      char* token = strtok_r(v, ",", &ptr);

      targets.push_back(atof(token));

      while((token = strtok_r(NULL, ",", &ptr)) != NULL){
	if(strcmp(token, "?") == 0){
	  targets.push_back(0.0);
	}
	else{
	  targets.push_back(atof(token));
	}
      }
    }
    else
      return false;

    if(strncmp(argv[5], "--target-var=", 13) == 0){
      char* v = argv[5] + 13;
      char* ptr = NULL;
      char* token = strtok_r(v, ",", &ptr);
      
      targetVar.push_back(atof(token));
      
      while((token = strtok_r(NULL, ",", &ptr)) != NULL){
	if(strcmp(token, "?") == 0){
	  targetVar.push_back(10e16); // very large value for this dimension
	}
	else{
	  targetVar.push_back(atof(token));
	}
      }
    }
    else
      return false; // unknown parameter

    if(targets.size() != 6 || targetVar.size() != 6)
      return false; // there should be six tokens in targets (interaxon muse)
  }

  
  DIR *dirh;
  struct dirent *ent;

  std::string& path = dir;

  if(path.size() <= 0)
    return false;
  
  if(path[path.size()-1] == '/' || path[path.size()-1] == '\\')
    path.resize(path.size()-1);
  
  
  if ((dirh = opendir (path.c_str())) != NULL) {
    
    while ((ent = readdir (dirh)) != NULL) {
      const char* filename = ent->d_name;
      const int L = strlen(filename);
      
      if(/*strcmp((const char*)(filename+L-4), ".jpg") == 0 || */
	 strcmp((const char*)(filename+L-4), ".png") == 0){
	
	std::string f = path;
	f += "/";
	f += ent->d_name;
	
	// std::cout << f << std::endl;
	
	pictures.push_back(f);
      }
      
      
    }
    closedir (dirh);
  }
  else return false;

  if(pictures.size() <= 0)
    return false;

  return true;
}
