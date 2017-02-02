
#include "ts_measure.h"

#include <SDL.h>
#include <chrono>

#include "pictureAutoencoder.h"

#include <map>


namespace whiteice{
  
  namespace resonanz{

    using namespace std::chrono;
    

    bool measureRandomPicturesAndTimeSeries
    (DataSource* dev,
     std::vector<std::string>& pictures,
     const unsigned int DISPLAYTIME,
     whiteice::dataset< whiteice::math::blas_real<double> >& timeseries)
    {
      if(dev == NULL || DISPLAYTIME == 0 || pictures.size() == 0)
	return false;

      // open window (SDL)

      SDL_Window* window = NULL;
      
      int W = 640;
      int H = 480;
      
      
      SDL_DisplayMode mode;
      
      if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
	// W = (3*mode.w)/4;
	// H = (3*mode.h)/4;
	W = mode.w;
	H = mode.h;
      }
      else return false;

      if(W > H) W = H;
      else if(H > W) H = W;

      window = SDL_CreateWindow("Time Series Analysis",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				W, H,
				SDL_WINDOW_ALWAYS_ON_TOP |
				SDL_WINDOW_INPUT_FOCUS);

      if(window == NULL)
	return false;
      
      SDL_RaiseWindow(window);
      SDL_UpdateWindowSurface(window);
      SDL_RaiseWindow(window);

      unsigned int picsize = H;
      if(W < H) picsize = W;

      // loads pictures
      std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > v;
      v.resize(pictures.size());

      SDL_Event event;
      bool exit = false;
      
      for(unsigned int i=0;i<v.size() && !exit;i++){
	if(picToVector(pictures[i], picsize, v[i], false) == false){
	  return false;
	}

	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
      }
	  

      while(!exit){
	
	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
	
	
	SDL_Surface* win = SDL_GetWindowSurface(window);

	SDL_FillRect(win, NULL, 
		     SDL_MapRGB(win->format, 0, 0, 0));
	
	SDL_Surface* scaled = NULL;	
	
	{ // displays picture from disk (downscaled to picsize) [75%]

	  const unsigned int r = rand() % pictures.size();

	  // printf("Load: %s (%d)\n", pictures[r].c_str(), picsize);
	  
	  if(vectorToSurface(v[r], picsize, scaled, false) == false){
	    continue;
	  }
	}

	SDL_Rect imageRect;
	
	if(win->w < win->h){
	  imageRect.w = win->w;
	  imageRect.h = win->w;
	  imageRect.x = 0;
	  imageRect.y = (H - win->w)/2;
	}
	else{
	  imageRect.w = win->h;
	  imageRect.h = win->h;
	  imageRect.x = (W - win->h)/2;
	  imageRect.y = 0;
	  
	}

	SDL_BlitScaled(scaled, NULL, win, &imageRect);

	SDL_FreeSurface(scaled);

	SDL_UpdateWindowSurface(window);
	SDL_ShowWindow(window);
	SDL_FreeSurface(win);

	unsigned int start_ms =
	  duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	
	std::vector<float> before, after;

	if(dev->connectionOk() == false){
	  printf("ERROR: dev->connectionOk() returned false\n");
	  return true;
	}

	if(dev->data(before) == false){
	  printf("ERROR: dev->data(before) returned false\n");
	  return false;
	}

	
	{
	  const unsigned int end_ms =
	    duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	  unsigned int delta_ms = end_ms - start_ms;

	  // printf("delta ms: %d (display ms: %d)\n", delta_ms, DISPLAYTIME);
	  
	  if(delta_ms < DISPLAYTIME){
	    usleep((DISPLAYTIME - delta_ms)*1000);
	  }
	  
	}

	if(dev->data(after) == false){
	  printf("ERROR: dev->data(after) returned false\n");
	  return false;
	}

	// stores measurement results
	{
	  whiteice::math::vertex< whiteice::math::blas_real<double> > a;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > b;

	  b.resize(before.size());
	  a.resize(after.size());
	  
	  for(unsigned int i=0;i<b.size();i++)
	    b[i] = before[i];

	  for(unsigned int i=0;i<a.size();i++)
	    a[i] = after[i];


	  // stores timeseries to dataset
	  bool timeseriesValid = false;

	  if(timeseries.getNumberOfClusters() == 1){
	    if(timeseries.dimension(0) == dev->getNumberOfSignals()){
	      timeseriesValid = true;
	    }
	  }


	  if(timeseriesValid == false)
	  {
	    timeseries.clear();
	    timeseries.createCluster("input", dev->getNumberOfSignals());
	    
	    timeseries.add(0, a);
	  }
	  else{
	    // just adds extra data
	  
	    timeseries.add(0, a);
	  }
	}
	
      }
      
      SDL_DestroyWindow(window);

            
      return true;
    }



    bool predictHiddenState(DataSource* dev,
			    const whiteice::HMM& hmm, 
			    std::vector<std::string>& pictures,
			    const unsigned int DISPLAYTIME,
			    whiteice::dataset< whiteice::math::blas_real<double> >& timeseries)
    {
      if(dev == NULL || DISPLAYTIME == 0 || pictures.size() == 0 || 
	 timeseries.getNumberOfClusters() != 1)
	return false;
      

      // initial distribution of states
      auto pi = hmm.getPI();
      unsigned int currentState = hmm.sample(pi);

      
      // discretizes observations
      std::vector<double> m, s;

      // calculates parameters (mean and standard deviation)
      {
	
	
	m.resize(dev->getNumberOfSignals());
	s.resize(dev->getNumberOfSignals());
	
	for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
	  m[i] = 0.0;
	  s[i] = 0.0;
	}
	
	for(unsigned int i=0;i<timeseries.size(0);i++){
	  auto& v = timeseries[i];
	  
	  for(unsigned int k=0;k<dev->getNumberOfSignals();k++){
	    m[k] += v[k].c[0];
	    s[k] += v[k].c[0]*v[k].c[0];
	  }
	}
	
	for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
	  m[i] /= ((double)timeseries.size(0));
	  s[i] /= ((double)timeseries.size(0));
	  
	  s[i] = m[i]*m[i];
	  s[i] = sqrt(fabs(s[i]));
	}
	
      }

      
      // open window (SDL)

      SDL_Window* window = NULL;
      
      int W = 640;
      int H = 480;
      
      
      SDL_DisplayMode mode;
      
      if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
	// W = (3*mode.w)/4;
	// H = (3*mode.h)/4;
	W = mode.w;
	H = mode.h;
      }
      else return false;

      if(W > H) W = H;
      else if(H > W) H = W;

      window = SDL_CreateWindow("Time Series Analysis",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				W, H,
				SDL_WINDOW_ALWAYS_ON_TOP |
				SDL_WINDOW_INPUT_FOCUS);

      if(window == NULL)
	return false;
      
      SDL_RaiseWindow(window);
      SDL_UpdateWindowSurface(window);
      SDL_RaiseWindow(window);
      
      unsigned int picsize = H;
      if(W < H) picsize = W;

      SDL_Event event;
      bool exit = false;
      
      // loads pictures
      std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > v;
      v.resize(pictures.size());
      
      for(unsigned int i=0;i<v.size() && !exit;i++){
	if(picToVector(pictures[i], picsize, v[i], false) == false){
	  return false;
	}

	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
	
      }


      while(!exit){
	
	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
	
	
	SDL_Surface* win = SDL_GetWindowSurface(window);

	SDL_FillRect(win, NULL, 
		     SDL_MapRGB(win->format, 0, 0, 0));
	
	SDL_Surface* scaled = NULL;	
	
	{ // displays picture from disk (downscaled to picsize) [75%]

	  const unsigned int r = rand() % pictures.size();

	  // printf("Load: %s (%d)\n", pictures[r].c_str(), picsize);
	  
	  if(vectorToSurface(v[r], picsize, scaled, false) == false){
	    continue;
	  }
	}

	SDL_Rect imageRect;
	
	if(win->w < win->h){
	  imageRect.w = win->w;
	  imageRect.h = win->w;
	  imageRect.x = 0;
	  imageRect.y = (H - win->w)/2;
	}
	else{
	  imageRect.w = win->h;
	  imageRect.h = win->h;
	  imageRect.x = (W - win->h)/2;
	  imageRect.y = 0;
	  
	}

	SDL_BlitScaled(scaled, NULL, win, &imageRect);

	SDL_FreeSurface(scaled);

	SDL_UpdateWindowSurface(window);
	SDL_ShowWindow(window);
	SDL_FreeSurface(win);

	unsigned int start_ms =
	  duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();      
	
	std::vector<float> before, after;
	
	if(dev->connectionOk() == false){
	  printf("ERROR: dev->connectionOk() returned false\n");
	  return true;
	}

	if(dev->data(before) == false){
	  printf("ERROR: dev->data(before) returned false\n");
	  return false;
	}

	
	{
	  const unsigned int end_ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	  
	  unsigned int delta_ms = end_ms - start_ms;
	  if(delta_ms < DISPLAYTIME){
	    usleep((DISPLAYTIME - delta_ms)*1000);
	  }
	  
	}

	if(dev->data(after) == false){
	  printf("ERROR: dev->data(after) returned false\n");
	  return false;
	}


	{
	  std::vector<double> afterd(after.size());

	  for(unsigned int i=0;i<afterd.size();i++)
	    afterd[i] = after[i];
	  
	  const unsigned int o = discretize(afterd, m, s);

	  unsigned int nextState = currentState;
	  
	  double p = hmm.next_state(currentState, nextState, o);

	  printf("HIDDEN STATE: %d\n", nextState);
	  fflush(stdout);

	  currentState = nextState;
	}
	
	
      }
      
      SDL_DestroyWindow(window);
      
      
      
    }

    
    
    unsigned int discretize(const std::vector<double>& data,
			    const std::vector<double>& m,
			    const std::vector<double>& s)
    {
      std::vector<double> o;
      
      for(unsigned int k=0;k<data.size();k++){
	const auto& x = data[k];

	if(fabs(x - m[k]) <= 0.5*s[k]){ // within 34% of the mean
	  o.push_back(0);
	}
	else if(x - m[k] <= -0.5*s[k]){
	  o.push_back(1);
	}
	else{ // (x - m[k] >= +0.5*s[k])
	  o.push_back(2);
	}
      }

      unsigned int state = 0; // observation
      unsigned int base = 1;

      for(unsigned int k=0;k<o.size();k++){
	state += o[k]*base;
	base = base*3;
      }

      return state;
    }
    
    
    bool measureRandomPictures(DataSource* dev,
			       const whiteice::HMM& hmm, 
			       std::vector<std::string>& pictures,
			       const unsigned int DISPLAYTIME,
			       whiteice::dataset< whiteice::math::blas_real<double> >& timeseries,
			       std::vector< whiteice::dataset< whiteice::math::blas_real<double> > >& data)
    {
      if(dev == NULL || DISPLAYTIME == 0 || pictures.size() == 0 || 
	 timeseries.getNumberOfClusters() != 1)
	return false;
      

      // initial distribution of states
      auto pi = hmm.getPI();
      unsigned int currentState = hmm.sample(pi);

      
      // discretizes observations
      std::vector<double> m, s;

      // calculates parameters (mean and standard deviation)
      {
	
	
	m.resize(dev->getNumberOfSignals());
	s.resize(dev->getNumberOfSignals());
	
	for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
	  m[i] = 0.0;
	  s[i] = 0.0;
	}
	
	for(unsigned int i=0;i<timeseries.size(0);i++){
	  auto& v = timeseries[i];
	  
	  for(unsigned int k=0;k<dev->getNumberOfSignals();k++){
	    m[k] += v[k].c[0];
	    s[k] += v[k].c[0]*v[k].c[0];
	  }
	}
	
	for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
	  m[i] /= ((double)timeseries.size(0));
	  s[i] /= ((double)timeseries.size(0));
	  
	  s[i] = m[i]*m[i];
	  s[i] = sqrt(fabs(s[i]));
	}
	
      }

      
      // open window (SDL)

      SDL_Window* window = NULL;
      
      int W = 640;
      int H = 480;
      
      
      SDL_DisplayMode mode;
      
      if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
	// W = (3*mode.w)/4;
	// H = (3*mode.h)/4;
	W = mode.w;
	H = mode.h;
      }
      else return false;

      if(W > H) W = H;
      else if(H > W) H = W;


      window = SDL_CreateWindow("Time Series Analysis",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				W, H,
				SDL_WINDOW_ALWAYS_ON_TOP |
				SDL_WINDOW_INPUT_FOCUS);

      if(window == NULL)
	return false;
      
      SDL_RaiseWindow(window);
      SDL_UpdateWindowSurface(window);
      SDL_RaiseWindow(window);

      unsigned int picsize = H;
      if(W < H) picsize = W;
      
      // loads pictures
      std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > v;
      v.resize(pictures.size());

      SDL_Event event;
      bool exit = false;

      
      for(unsigned int i=0;i<v.size() && !exit;i++){
	if(picToVector(pictures[i], picsize, v[i], false) == false){
	  return false;
	}

	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
      }

      
      
      while(!exit){
	
	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
	
	
	SDL_Surface* win = SDL_GetWindowSurface(window);

	SDL_FillRect(win, NULL, 
		     SDL_MapRGB(win->format, 0, 0, 0));
	
	SDL_Surface* scaled = NULL;

	const unsigned int r = rand() % pictures.size();
	
	{ // displays picture from disk (downscaled to picsize) [75%]

	  // printf("Load: %s (%d)\n", pictures[r].c_str(), picsize);
	  
	  if(vectorToSurface(v[r], picsize, scaled, false) == false){
	    continue;
	  }
	  
	}

	SDL_Rect imageRect;
	
	if(win->w < win->h){
	  imageRect.w = win->w;
	  imageRect.h = win->w;
	  imageRect.x = 0;
	  imageRect.y = (H - win->w)/2;
	}
	else{
	  imageRect.w = win->h;
	  imageRect.h = win->h;
	  imageRect.x = (W - win->h)/2;
	  imageRect.y = 0;
	  
	}

	SDL_BlitScaled(scaled, NULL, win, &imageRect);

	SDL_FreeSurface(scaled);

	SDL_UpdateWindowSurface(window);
	SDL_ShowWindow(window);
	SDL_FreeSurface(win);

	unsigned int start_ms =
	  duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	
	std::vector<float> before, after;
	
	if(dev->connectionOk() == false){
	  printf("ERROR: dev->connectionOk() returned false\n");
	  return true;
	}

	if(dev->data(before) == false){
	  printf("ERROR: dev->data(before) returned false\n");
	  return false;
	}

	
	{
	  const unsigned int end_ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	  
	  unsigned int delta_ms = end_ms - start_ms;
	  if(delta_ms < DISPLAYTIME){
	    usleep((DISPLAYTIME - delta_ms)*1000);
	  }
	  
	}

	if(dev->data(after) == false){
	  printf("ERROR: dev->data(after) returned false\n");
	  return false;
	}

	// adds measurement data
	{
	  auto& store = data[r];

	  if(store.getNumberOfClusters() != 2){
	    store.clear();

	    if(store.createCluster("before", before.size() + hmm.getNumHiddenStates()) == false)
	      return false;
	    
	    if(store.createCluster("after", after.size()) == false)
	      return false;
	  }

	  whiteice::math::vertex< whiteice::math::blas_real<double> > dat;
	  dat.resize(before.size() + hmm.getNumHiddenStates());
	  dat.zero();

	  for(unsigned int i=0;i<before.size();i++)
	    dat[i] = before[i];

	  // sets indicator variable for hidden state
	  dat[before.size() + currentState] = 1.0;

	  if(store.add(0, dat) == false) return false;
	  
	  dat.resize(after.size());
	  dat.zero();
	  
	  for(unsigned int i=0;i<after.size();i++)
	    dat[i] = (after[i] - before[i])/((double)DISPLAYTIME);

	  if(store.add(1, dat) == false) return false;
	}


	// updates hidden state
	{
	  std::vector<double> afterd(after.size());

	  for(unsigned int i=0;i<afterd.size();i++)
	    afterd[i] = after[i];
	  
	  const unsigned int o = discretize(afterd, m, s);

	  unsigned int nextState = currentState;
	  
	  double p = hmm.next_state(currentState, nextState, o);
	  
	  currentState = nextState;
	}
	
	
      }
      
      SDL_DestroyWindow(window);      

      return true;
    }


    bool optimizeNeuralnetworkModel(const whiteice::dataset< whiteice::math::blas_real<double> >& data,
				    whiteice::nnetwork< whiteice::math::blas_real<double> >& net,
				    double& netError)
    {
      netError = 0.0;
      
      if(data.getNumberOfClusters() != 2) return false;
      if(data.size(0) <= 0 || data.size(1) <= 1) return false;
      
      const unsigned int inputDimension  = data.dimension(0);
      const unsigned int outputDimension = data.dimension(1);

      std::vector<unsigned int> arch;

      // creates deep 5-layer neural network
      arch.push_back(inputDimension);
      arch.push_back(inputDimension*10);
      arch.push_back(inputDimension*10);
      arch.push_back(inputDimension*10);
      arch.push_back(inputDimension*10);
      arch.push_back(outputDimension);



      whiteice::math::vertex< whiteice::math::blas_real<double> > w;

      net.setArchitecture(arch,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >::halfLinear);
      net.setNonlinearity(net.getLayers()-1,
			  whiteice::nnetwork< whiteice::math::blas_real<double> >::pureLinear);

      printf("NNETWORK: ");
      for(unsigned int i=1;i<arch.size();i++){
	printf("%dx%d ", arch[i-1], arch[i]);
      }      
      printf(" (%d elements)\n", net.exportdatasize());
      
      net.randomize();
      net.exportdata(w);

      // auto* optimizer = new whiteice::pLBFGS_nnetwork<whiteice::math::blas_real<double> >(net, data, false, false);
      // optimizer->minimize(4); // uses 4 threads

      auto* optimizer = new whiteice::math::NNGradDescent<whiteice::math::blas_real<double> >();
      optimizer->startOptimize(data, net, 4);

      unsigned int iterations = 0;
      whiteice::math::blas_real<double> error = 1000.0f;
      
      do{
	const unsigned int old_iters = iterations;

	// optimizer->getSolution(w, error, iterations);
	optimizer->getSolution(net, error, iterations);
	
	if(old_iters != iterations){
	  printf("OPTIMIZATION ERROR: %e (%d iters)\n", error.c[0], iterations);
	  fflush(stdout);
	}
	
	sleep(1); // checks status every 1 sec
      }
      while(iterations < 1000);

      optimizer->stopComputation();
      // optimizer->getSolution(w, error, iterations);
      optimizer->getSolution(net, error, iterations);

      // net.importdata(w);

      delete optimizer;

      netError = error.c[0];

      return true;
    }


    bool stimulateUsingModel(DataSource* dev,
			     const whiteice::HMM& hmm,
			     const std::vector< whiteice::bayesian_nnetwork< whiteice::math::blas_real<double> > >& nets,
			     const std::vector<std::string>& pictures,
			     const unsigned int DISPLAYTIME,
			     const whiteice::dataset< whiteice::math::blas_real<double> >& timeseries,
			     const std::vector<double>& target,
			     const std::vector<double>& targetVar,
			     bool random)
    {

      if(dev == NULL || DISPLAYTIME == 0 || pictures.size() == 0 || 
	 timeseries.getNumberOfClusters() != 1)
	return false;

      std::vector<double> targetErrors; // error (distance) to target for each step
      

      // initial distribution of states
      auto pi = hmm.getPI();
      unsigned int currentState = hmm.sample(pi);

      
      // discretizes observations
      std::vector<double> m, s;

      // calculates parameters (mean and standard deviation)
      {
	
	
	m.resize(dev->getNumberOfSignals());
	s.resize(dev->getNumberOfSignals());
	
	for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
	  m[i] = 0.0;
	  s[i] = 0.0;
	}
	
	for(unsigned int i=0;i<timeseries.size(0);i++){
	  auto& v = timeseries[i];
	  
	  for(unsigned int k=0;k<dev->getNumberOfSignals();k++){
	    m[k] += v[k].c[0];
	    s[k] += v[k].c[0]*v[k].c[0];
	  }
	}
	
	for(unsigned int i=0;i<dev->getNumberOfSignals();i++){
	  m[i] /= ((double)timeseries.size(0));
	  s[i] /= ((double)timeseries.size(0));
	  
	  s[i] = m[i]*m[i];
	  s[i] = sqrt(fabs(s[i]));
	}
	
      }

      
      // open window (SDL)

      SDL_Window* window = NULL;
      
      int W = 640;
      int H = 480;
      
      
      SDL_DisplayMode mode;
      
      if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
	// W = (3*mode.w)/4;
	// H = (3*mode.h)/4;
	W = mode.w;
	H = mode.h;
      }
      else return false;

      if(W > H) W = H;
      else if(H > W) H = W;

      window = SDL_CreateWindow("Time Series Analysis",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				W, H,
				SDL_WINDOW_ALWAYS_ON_TOP |
				SDL_WINDOW_INPUT_FOCUS);

      if(window == NULL)
	return false;
      
      SDL_RaiseWindow(window);
      SDL_UpdateWindowSurface(window);
      SDL_RaiseWindow(window);

      unsigned int picsize = H;
      if(W < H) picsize = W;

      // loads pictures
      std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > v;
      v.resize(pictures.size());

      SDL_Event event;
      bool exit = false;
      
      for(unsigned int i=0;i<v.size() && !exit;i++){
	if(picToVector(pictures[i], picsize, v[i], false) == false){
	  return false;
	}

	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}
      }

      unsigned start_ms =
	duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
      
      std::vector<float> before, after;
      
      if(dev->connectionOk() == false){
	printf("ERROR: dev->connectionOk() returned false\n");
	return true;
      }

      if(dev->data(before) == false){
	printf("ERROR: dev->data(after) returned false\n");
	return false;
      }

      after = before;

      
      
      while(!exit){
	
	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    exit = true;
	    continue;
	  }
	}


	unsigned int r = rand() % pictures.size();
	
	// selects next picture that should move closest to the target state
	{
	  double best_error = 1000000.0;
	  
	  std::vector<double> errors;
	  errors.resize(nets.size());

	  bool failure = false;
	  
			
#pragma omp parallel for schedule(dynamic) 
	  for(unsigned int i=0;i<nets.size();i++){

	    whiteice::math::vertex< whiteice::math::blas_real<double> > v;
	    
	    v.resize(before.size() + hmm.getNumHiddenStates());
	    v.zero();

	    for(unsigned int i=0;i<before.size();i++)
	      v[i] = before[i];

	    // sets indicator variable for hidden state
	    v[before.size() + currentState] = 1.0;

	    whiteice::nnetwork< whiteice::math::blas_real<double> > nn;
	    std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > weights;
	    
	    if(nets[i].exportSamples(nn, weights) == false){
	      printf("ERROR: accessing nnetwork %d/%d failed\n", i+1, nets.size());
	      failure = true;
	      continue;
	    }

	    if(weights.size() < 1){
	      printf("ERROR: accessing nnetwork %d/%d failed\n", i+1, nets.size());
	      failure = true;
	      continue;
	    }

	    if(nn.importdata(weights[0]) == false){
	      printf("ERROR: accessing nnetwork %d/%d failed\n", i+1, nets.size());
	      failure = true;
	      continue;
	    }

	    whiteice::math::vertex< whiteice::math::blas_real<double> > out;
	    out.resize(before.size());
	    out.zero();

	    if(nn.calculate(v, out) == false){
	      printf("ERROR: nnetwork.calculate() %d/%d failed\n", i+1, nets.size());
	      failure = true;
	      continue;
	    }

	    if(out.size() != target.size()){
	      printf("ERROR: nnetwork.calculate() output mismatch %d/%d\n",
		     i+1, nets.size());
	      failure = true;
	      continue;
	    }

	    for(unsigned int j=0;j<out.size();j++){
	      out[j] = out[j]*((double)DISPLAYTIME) + before[j];
	    }

	    double err = 0.0;

	    for(unsigned int j=0;j<out.size();j++){
	      err += (out[j].c[0]-target[j])*(out[j].c[0]-target[j])/targetVar[j];
	    }

	    err = sqrt(err);

	    errors[i] = err;
	  }

	  if(failure) return false; // internal failure

	  // selects the smallest error
	  {
	    std::multimap<double, unsigned int> orderedList;
	    
	    for(unsigned int i=0;i<errors.size();i++){
	      orderedList.insert( std::pair<double, unsigned int>(errors[i], i));
	    }

	    while(orderedList.size() > 3){
	      orderedList.erase( std::prev( orderedList.end() ) );
	    }

	    unsigned int selected = rand() % orderedList.size();

	    auto e = orderedList.begin();

	    while(selected > 0){
	      e++;
	      selected--;
	    }

	    r = e->second;
	    best_error = e->first;
	  }

	  if(random)
	    r = rand() % pictures.size();
	}

	
	{
	  const unsigned int end_ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	  
	  unsigned int delta_ms = end_ms - start_ms;

	  // printf("delta ms: %d (DISPLAYTIME: %d)\n", delta_ms, DISPLAYTIME);
	  
	  if(delta_ms < DISPLAYTIME){
	    usleep((DISPLAYTIME - delta_ms)*1000);
	  }
	}
	
	
	
	// displays next picture picture
	{
	  SDL_Surface* win = SDL_GetWindowSurface(window);
	  
	  SDL_FillRect(win, NULL, 
		       SDL_MapRGB(win->format, 0, 0, 0));
	  
	  SDL_Surface* scaled = NULL;	
	  
	  { // displays picture from disk (downscaled to picsize) [75%]
	    
	    // printf("Load: %s (%d)\n", pictures[r].c_str(), picsize);
	    
	    if(vectorToSurface(v[r], picsize, scaled, false) == false){
	      continue;
	    }
	  }
	  
	  SDL_Rect imageRect;
	  
	  if(win->w < win->h){
	    imageRect.w = win->w;
	    imageRect.h = win->w;
	    imageRect.x = 0;
	    imageRect.y = (H - win->w)/2;
	  }
	  else{
	    imageRect.w = win->h;
	    imageRect.h = win->h;
	    imageRect.x = (W - win->h)/2;
	    imageRect.y = 0;
	    
	  }
	  
	  SDL_BlitScaled(scaled, NULL, win, &imageRect);
	  
	  SDL_FreeSurface(scaled);
	  
	  SDL_UpdateWindowSurface(window);
	  SDL_ShowWindow(window);
	  SDL_FreeSurface(win);
	}
	
	
	start_ms =
	  duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	
	
	if(dev->connectionOk() == false){
	  printf("ERROR: dev->connectionOk() returned false\n");
	  return true;
	}

	if(dev->data(after) == false){
	  printf("ERROR: dev->data(after) returned false\n");
	  return false;
	}

	before = after;

	// calculates current error (distance from target state)
	{
	  double err = 0.0;
	  
	  for(unsigned int j=0;j<after.size();j++){
	    err += (after[j]-target[j])*(after[j]-target[j])/targetVar[j];
	  }
	  
	  err = sqrt(err);

	  targetErrors.push_back(err);
	}
	
	// updates hidden state
	{
	  std::vector<double> stated(before.size());

	  for(unsigned int i=0;i<stated.size();i++)
	    stated[i] = before[i];
	  
	  const unsigned int o = discretize(stated, m, s);

	  unsigned int nextState = currentState;
	  
	  double p = hmm.next_state(currentState, nextState, o);
	  
	  currentState = nextState;
	}
	
      }
      
      
      SDL_DestroyWindow(window);

      // reports average error during stimulation sequence
      {
	double E = 0.0;
	
	for(auto& e : targetErrors){
	  E += e / ((double)targetErrors.size());
	}

	printf("Average error during stimulation: %f\n", E);
      }

      

      return true;
      
    }
    
    
  }
}
