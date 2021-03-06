

#include "ReinforcementPictures.h"

#include <thread>
#include <functional>
#include <exception>
#include <chrono>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <unistd.h>


// defines size of feature vector square picture (mini picture)
// that is calculated from actual pictures (was: 32x32)
// FEATURE_PICSIZE x FEATURE_PICSIZE * 3 (RGB) sized vector
#define FEATURE_PICSIZE 32 // minimal 4x4 picture



namespace whiteice
{
  using namespace std::chrono;
  using namespace whiteice::resonanz;

  template <typename T>
  ReinforcementPictures<T>::ReinforcementPictures
  (const DataSource* dev,
   const whiteice::HMM& hmm,
   const whiteice::KMeans< T >& clusters,
   const whiteice::bayesian_nnetwork<T>& rmodel,
   const std::vector<std::string>& pictures,
   const unsigned int DISPLAYTIME,
   const std::vector<double>& target,
   const std::vector<double>& targetVar) :
    RIFL_abstract<T>(pictures.size(),
		     dev == NULL ? 0 :
		     dev->getNumberOfSignals() + hmm.getNumHiddenStates(),
		     FEATURE_PICSIZE*FEATURE_PICSIZE*3)
  {
    whiteice::logging.info("RP::ctor() starting..");
    
    if(dev == NULL || DISPLAYTIME == 0 || pictures.size() == 0 || clusters.size() == 0)
    {
      throw std::invalid_argument("ReinforcementPictures ctor: bad arguments");
    }

    keypresses = 0;
    esc_keypresses = 0;
    
    display_thread = nullptr;

    this->dev = dev;
    this->hmm = hmm;
    this->clusters = clusters;
    this->rmodel = rmodel;
    this->pictures = pictures;
    this->DISPLAYTIME = DISPLAYTIME;
    this->target = target;
    this->targetVar = targetVar;

    this->currentHMMstate = 0;

    this->fontname = "Vera.ttf";

    this->random = false;

    this->useReinforcementModel = false;

    this->initialized = false;

    // starts display thread
    {
      whiteice::logging.info("Starting display thread..");
		 
      running = true;
      display_thread = new std::thread(std::bind(&ReinforcementPictures<T>::displayLoop,
						 this));
    }
  }

  template <typename T>
  ReinforcementPictures<T>::~ReinforcementPictures()
  {
    // stops display thread
    {
      std::lock_guard<std::mutex> lock(display_mutex);

      running = false;
      display_thread->join();
    }
  }

  
  template <typename T>
  bool ReinforcementPictures<T>::getKeypress()
  {
    bool press = (keypresses > 0);
    keypresses = 0;
    return press;
  }


  template <typename T>
  bool ReinforcementPictures<T>::getESCKeypress()
  {
    bool press = (esc_keypresses > 0);
    esc_keypresses = 0;
    return press;
  }


  template <typename T>
  bool ReinforcementPictures<T>::getDisplayIsRunning()
  {
    return (this->running);
  }

  template <typename T>
  bool ReinforcementPictures<T>::getInitialized()
  {  // return true when we are in main display loop and all things are set properly
    return initialized;
  }


  template <typename T>
  void ReinforcementPictures<T>::setRandom(bool r)
  {
    random = r;
  }


  // sets message that will be shown on the screen
  template <typename T>
  void ReinforcementPictures<T>::setMessage(const std::string& msg)
  {
    message = msg;
  }


  template <typename T>
  void ReinforcementPictures<T>::setReinforcementModel(bool useModel)
  {
    useReinforcementModel = useModel;
  }
  

  template <typename T>
  bool ReinforcementPictures<T>::getState(whiteice::math::vertex<T>& state)
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
  bool ReinforcementPictures<T>::getActionFeature(const unsigned int action,
						  whiteice::math::vertex<T>& feature) const
  {
    feature.resize(this->dimActionFeatures);
    feature.zero();
    
    if(action >= actionFeatures.size()) return false;

    if(actionFeatures[action].size() == this->dimActionFeatures){
      // adds random noise to feature vectors (less overfitting)
      rng.uniform(feature);
      for(unsigned int i=0;i<feature.size();i++){
	feature[i] = (T(4.0/255.0)*feature[i] - T(2.0/255.0)) +
	  actionFeatures[action][i];

	if(feature[i] < T(0.0)) feature[i] = T(0.0);
	else if(feature[i] > T(1.0)) feature[i] = T(1.0);
      }
    }
    else{
      return false;
    }

    return true;
  }

  template <typename T>
  bool ReinforcementPictures<T>::performAction(const unsigned int action,
					       whiteice::math::vertex<T>& newstate,
					       T& reinforcement)
  {

    // adds action to the queue (the next action)
    {
      std::lock_guard<std::mutex> lock(actionMutex);
      actionQueue.push_back(action);
    }
    
    // waits for action execution to start (FIXME use condition varibles for this..)
    {
      while(running){
	{
	  std::lock_guard<std::mutex> lock(actionMutex);
	  if(actionQueue.size() == 0)
	    break;
	}
	
	usleep(10);
      }

      if(running == false)
	return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DISPLAYTIME/2));
    // usleep(DISPLAYTIME*1000/2); // we can safely sleep 50% of the display time

    // waits for action to be added to old actions queue and clears it
    // FIXME: change busy loop to condition variables
    {
      while(running){
	{
	  std::lock_guard<std::mutex> lock(performedActionsMutex);
	  if(performedActionsQueue.size() > 0){
	    performedActionsQueue.pop_front();
	    break;
	  }
	}
	
	usleep(10);
      }

      if(running == false)
	return false;
    }

    // gets new state
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

	// minus: we sought to minimize distance to the target state (maximize -f(x))
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
	  printf("PIC GOODNESS-150: %f +- %f (HAS MODEL: %d)\n",
		 d150.c[0], dev.c[0], this->getHasModel());
	}
      }

    }
    
    
    return true;
  }



  /*
   * calculates mini feature vectors (mini pictures)
   * from images
   */
  template <typename T>
  void ReinforcementPictures<T>::calculateFeatureVector(SDL_Surface* pic,
							whiteice::math::vertex<T>& f) const
  {
    f.resize(FEATURE_PICSIZE*FEATURE_PICSIZE*3);
    f.zero();
    
    if(pic == NULL){
      return;
    }
    
    SDL_Surface* scaled = NULL;
    
    scaled = SDL_CreateRGBSurface(0, FEATURE_PICSIZE, FEATURE_PICSIZE, 32,
				  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if(scaled == NULL) return;

    SDL_Rect srcrect;
    
    if(pic->w < pic->h){
      srcrect.w = pic->w;
      srcrect.x = 0;
      srcrect.h = pic->w;
      srcrect.y = (pic->h - pic->w)/2;
    }
    else{
      srcrect.h = pic->h;
      srcrect.y = 0;
      srcrect.w = pic->h;
      srcrect.x = (pic->w - pic->h)/2;
      }
    
    if(SDL_BlitScaled(pic, &srcrect, scaled, NULL) != 0){
      SDL_FreeSurface(scaled);
      return;
    }

    unsigned int index = 0;
    
    for(int j=0;j<scaled->h;j++){
      for(int i=0;i<scaled->w;i++){

	unsigned int pixel = ((unsigned int*)(((char*)scaled->pixels) + j*scaled->pitch))[i];
	unsigned int r = (0x00FF0000 & pixel) >> 16;
	unsigned int g = (0x0000FF00 & pixel) >>  8;
	unsigned int b = (0x000000FF & pixel);
	
	f[index] = (double)r/255.0; index++;
	f[index] = (double)g/255.0; index++;
	f[index] = (double)b/255.0; index++;
	
      }
    }

    SDL_FreeSurface(scaled);

    return; // everything OK
  }
  

  template <typename T>
  void ReinforcementPictures<T>::displayLoop()
  {
#if 0
    // initialization
    logging.info("Starting SDL init (0)..");

    SDL_Init(0);

    logging.info("Starting SDL subsystem init (events, video, audio)..");

    if(SDL_InitSubSystem(SDL_INIT_EVENTS) != 0){
      logging.error("SDL_Init(EVENTS) failed.");
      SDL_Quit();
      running = false;
      return;
    }

    if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0){
      logging.error("SDL_Init(VIDEO) failed.");
      SDL_Quit();
      running = false;
      return;
    }

    if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0){
      logging.error("SDL_Init(AUDIO) failed.");
      SDL_Quit();
      running = false;
      return;
    }

    logging.info("SDL_Init() events, video, audio successful.");
#endif
    
    // opens SDL display

    SDL_Window* window = NULL;

    keypresses = 0;
    esc_keypresses = 0;
    
    int W = 640;
    int H = 480;

    SDL_DisplayMode mode;

    if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
      W = (3*mode.w)/4;
      H = (3*mode.h)/4;
    }
    else{
      whiteice::logging.error("SDL_GetCurrentDisplayMode() failed");
      running = false;
      SDL_Quit();
      return;
    }

#if 0
    if(TTF_Init() != 0){
      char buffer[80];
      snprintf(buffer, 80, "TTF_Init failed: %s\n", TTF_GetError());
      logging.error(buffer);
      TTF_Quit();
      SDL_Quit();      
      running = false;      
      return;
    }
    else
      logging.info("Starting TTF_Init() done..");

    int flags = IMG_INIT_JPG | IMG_INIT_PNG;

    if(IMG_Init(flags) != flags){
      char buffer[80];
      snprintf(buffer, 80, "IMG_Init failed: %s\n", IMG_GetError());
      logging.error(buffer);
      IMG_Quit();
      TTF_Quit();
      SDL_Quit();
      running = false;
      return;
    }
#endif

    window = SDL_CreateWindow("Tranquility",
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      W, H,
			      SDL_WINDOW_ALWAYS_ON_TOP |
			      SDL_WINDOW_INPUT_FOCUS);

    if(window == NULL){
      whiteice::logging.error("SDL_CreateWindow() failed\n");
      running = false;
      return;
    }

    SDL_RaiseWindow(window);
    SDL_UpdateWindowSurface(window);
    SDL_RaiseWindow(window);

    // loads font
    TTF_Font* font  = NULL;
    TTF_Font* font2 = NULL;

    {
      double fontSize = 100.0*sqrt(((float)(W*H))/(640.0*480.0));
      unsigned int fs = (unsigned int)fontSize;
      if(fs <= 0) fs = 10;
      
      font = TTF_OpenFont(fontname.c_str(), fs);
      
      fontSize = 25.0*sqrt(((float)(W*H))/(640.0*480.0));
      fs = (unsigned int)fontSize;
      if(fs <= 0) fs = 10;

      font2 = TTF_OpenFont(fontname.c_str(), fs);
    }


    // loads all pictures
    std::vector<SDL_Surface*> images;
    images.resize(pictures.size());

    actionFeatures.resize(pictures.size());
    
    {
      for(unsigned int i=0;i<pictures.size();i++)
	images[i] = NULL;

      unsigned int numLoaded = 0;
      
#pragma omp parallel for shared(images) shared(numLoaded) schedule(dynamic)
      for(unsigned int i=0;i<pictures.size();i++)
      {
	if(running == false) continue;

	SDL_Surface* image = NULL;

#pragma omp critical
	{
	  // IMG_loader functions ARE NOT thread-safe so
	  // we cannot load files parallel
	  image = IMG_Load(pictures[i].c_str());
	}
	

	if(image == NULL){
	  char buffer[120];
	  snprintf(buffer, 120, "Loading image FAILED (%s): %s",
		   SDL_GetError(), pictures[i].c_str());
	  whiteice::logging.warn(buffer);
	  printf("ERROR: %s\n", buffer);

	  image = SDL_CreateRGBSurface(0, W, H, 32,
				       0x00FF0000, 0x0000FF00, 0x000000FF,
				       0xFF000000);

	  if(image == NULL){
	    whiteice::logging.error("Creating RGB surface failed");
	    running = false;
	    continue;
	  }
	  
	  SDL_FillRect(image, NULL, SDL_MapRGB(image->format, 0, 0, 0));
	}

	SDL_Rect imageRect;
	SDL_Surface* scaled = NULL;

	if(image->w >= image->h){
	  double wscale = ((double)W)/((double)image->w);
	  
	  scaled = SDL_CreateRGBSurface(0,
					(int)(image->w*wscale),
					(int)(image->h*wscale), 32,
					0x00FF0000, 0x0000FF00, 0x000000FF,
					0xFF000000);

	  if(scaled == NULL){
	    whiteice::logging.error("Creating RGB surface failed");
	    running = false;
	    continue;
	  }

	  if(SDL_BlitScaled(image, NULL, scaled, NULL) != 0)
	    whiteice::logging.warn("SDL_BlitScaled fails");
	}
	else{
	  double hscale = ((double)H)/((double)image->h);
	  
	  scaled = SDL_CreateRGBSurface(0,
					(int)(image->w*hscale),
					(int)(image->h*hscale), 32,
					0x00FF0000, 0x0000FF00, 0x000000FF,
					0xFF000000);

	  if(scaled == NULL){
	    whiteice::logging.error("Creating RGB surface failed");
	    running = false;
	    continue;
	  }
	  
	  if(SDL_BlitScaled(image, NULL, scaled, NULL) != 0)
	    whiteice::logging.warn("SDL_BlitScaled fails");
	}

	images[i] = scaled;

	if(image) SDL_FreeSurface(image);

	// creates feature vector (mini picture) of the image
	{
	  actionFeatures[i].resize(this->dimActionFeatures);
	  calculateFeatureVector(images[i], actionFeatures[i]);
	}

	numLoaded++;

	// displays pictures that are being loaded
#pragma omp critical
	{
	  imageRect.w = scaled->w;
	  imageRect.h = scaled->h;
	  imageRect.x = (W - scaled->w)/2;
	  imageRect.y = (H - scaled->h)/2;

	  SDL_Surface* surface = SDL_GetWindowSurface(window);

	  if(surface){
	    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
	    SDL_BlitSurface(scaled, NULL, surface, &imageRect);

	    if(font){
	      SDL_Color white = { 255, 255, 255 };
	      
	      char message[80];
	      snprintf(message, 80, "%d/%d", numLoaded, pictures.size());
	      
	      SDL_Surface* msg = TTF_RenderUTF8_Blended(font, message, white);
	      
	      if(msg != NULL){
		SDL_Rect messageRect;
		
		messageRect.x = (W - msg->w)/2;
		messageRect.y = (H - msg->h)/2;
		messageRect.w = msg->w;
		messageRect.h = msg->h;
		
		SDL_BlitSurface(msg, NULL, surface, &messageRect);
		
		SDL_FreeSurface(msg);
	      }
	    }

	    SDL_UpdateWindowSurface(window);
	    SDL_ShowWindow(window);
	    SDL_FreeSurface(surface);
	  }
	}

	SDL_Event event;

	while(SDL_PollEvent(&event)){
	  if(event.type == SDL_KEYDOWN){
	    keypresses++;
	    
	    if(event.key.keysym.sym == SDLK_ESCAPE)
	      esc_keypresses++;

	    continue;
	  }
	}

      }

    }
    

    long long start_ms =
      duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();

    unsigned int index = 0;

    // selects HMM initial state randomly according to starting state probabilities
    auto pi = hmm.getPI();
    currentHMMstate = hmm.sample(pi);
    
    
    while(running){
      initialized = true;
      
      SDL_Event event;

      while(SDL_PollEvent(&event)){
	if(event.type == SDL_KEYDOWN){
	  keypresses++;

	  if(event.key.keysym.sym == SDLK_ESCAPE)
	    esc_keypresses++;
	  
	  continue;
	}
      }

      if(dev->connectionOk() == false){
	whiteice::logging.info("ReinforcementPictures: Device connection failed");
	running = false;
	continue;
      }

      // waits for full picture show time
      {
	const long long end_ms =
	  duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
	long long delta_ms = end_ms - start_ms;

	if(delta_ms < DISPLAYTIME){
	  std::this_thread::sleep_for(std::chrono::milliseconds(DISPLAYTIME - delta_ms));
	  // usleep((DISPLAYTIME - delta_ms)*1000);
	}
      }

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
      {
	std::lock_guard<std::mutex> lock(performedActionsMutex);
	performedActionsQueue.push_back(index);
      }
      
      
      
      // waits for a command (picture) to show
      index = 0;
      {
	while(running){
	  
	  {
	    std::lock_guard<std::mutex> lock(actionMutex);
	    if(actionQueue.size() > 0){
	      index = actionQueue.front();
	      actionQueue.pop_front();
	      break;
	    }
	  }
	  usleep(10);
	}

	if(!running) continue;
      }

      // if we are in random mode we choose random pictures
      if(random)
	index = rng.rand() % images.size();

      
      {
	SDL_Surface* surface = SDL_GetWindowSurface(window);

	if(surface != NULL){
	  SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
	  
	  SDL_Rect imageRect;
	  imageRect.w = images[index]->w;
	  imageRect.h = images[index]->h;
	  imageRect.x = (W - images[index]->w)/2;
	  imageRect.y = (H - images[index]->h)/2;
	  
	  SDL_BlitSurface(images[index], NULL, surface, &imageRect);

	  if(font2 && message.size() > 0){
	    SDL_Color white = { 255, 255, 255 };
	    
	    SDL_Surface* msg = TTF_RenderUTF8_Blended(font2,
						      message.c_str(),
						      white);
	    
	    if(msg != NULL){
	      SDL_Rect messageRect;
	      
	      messageRect.x = (W - msg->w)/2;
	      messageRect.y = (H - msg->h)/2;
	      messageRect.w = msg->w;
	      messageRect.h = msg->h;
	      
	      SDL_BlitSurface(msg, NULL, surface, &messageRect);
	      
	      SDL_FreeSurface(msg);
	    }
	  }

	  SDL_UpdateWindowSurface(window);
	  SDL_ShowWindow(window);
	  SDL_FreeSurface(surface);
	}
      }

      start_ms =
	duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
      
    }

    
    SDL_DestroyWindow(window);

    if(font){
      TTF_CloseFont(font);
      font = NULL;
    }

    if(font2){
      TTF_CloseFont(font2);
      font2 = NULL;
    }

#if 0
    logging.info("SDL deinitialization started");
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    logging.info("SDL deinitialization.. DONE");
#endif
    
  }
  
  
  
  template class ReinforcementPictures< math::blas_real<float> >;
  template class ReinforcementPictures< math::blas_real<double> >;
};
