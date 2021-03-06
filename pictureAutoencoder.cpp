
#include "pictureAutoencoder.h"

#include <dinrhiw.h>
#include <SDL.h>
#include <SDL_image.h>

#include "hsv.h"

namespace whiteice
{
  namespace resonanz {

    
    // optimizes/learns autoencoder for processing pictures
    bool learnPictureAutoencoder(const std::string& picdir,
				 std::vector<std::string>& pictures,
				 unsigned int picsize,
				 whiteice::dataset< whiteice::math::blas_real<double> >& preprocess,
				 whiteice::nnetwork< whiteice::math::blas_real<double> >*& encoder,
				 whiteice::nnetwork< whiteice::math::blas_real<double> >*& decoder)
    {
      // 1. converts pictures to vectors and trains DBN

      std::vector< whiteice::math::vertex< whiteice::math::blas_real<double> > > data;

      // data.resize(pictures.size());
      
      unsigned int sum = 0;
      unsigned int counter = 0;
      
      // #pragma omp parallel for shared(sum)
      for(counter=0;counter<pictures.size();counter++){
      	
	whiteice::math::vertex< whiteice::math::blas_real<double> > v;
	const auto& p = pictures[counter];
	
	if(picToVector(p, picsize, v) == false){
	  printf("Cannot load picture: %s\n", pictures[counter].c_str());
	}
	else{
	  data.push_back(v);
	  // data[counter] = v;
	  sum++;
	}
      }

      if(data.size() <= 0){
	return false;
      }

      // preprocessing
      if(preprocess.createCluster("data", data[0].size()) == false){
	return false;
      }

      // preprocessing..
      {
	if(preprocess.add(0, data) == false) return false;
	
	preprocess.preprocess(0,whiteice::dataset< whiteice::math::blas_real<double> >::dnMeanVarianceNormalization);
	// preprocess.preprocess(0,whiteice::dataset< whiteice::math::blas_real<double> >::dnCorrelationRemoval);
	preprocess.clearData(0);
	
	// preprocesses data using 
	if(preprocess.preprocess(0, data) == false) return false;
      }

      // 2. trains DBN given data
      std::vector<unsigned int> arch; // architecture of DBN
      arch.push_back(3 * picsize * picsize); // 48
      arch.push_back(100 * 3 * picsize * picsize); // 4800: feature extraction layer (SLOW)
      arch.push_back(50); // 1024 different pictures.. (out of 2**16 = 65535 grayscale pictures)
      
      whiteice::DBN< whiteice::math::blas_real<double> > dbn(arch);
      dbn.initializeWeights();
      
      whiteice::math::blas_real<double> error = 0.01;

      // does DBN pretraining..
      if(dbn.learnWeights(data, error, true) == false)
	return false; // training failed

      // after pre-training dbn we convert DBN into feedforward autoencoder and train it

      whiteice::nnetwork< whiteice::math::blas_real<double> >* net = nullptr;
      
      if(dbn.convertToAutoEncoder(net) == false)
	return false; // if false is returned then net is always null

#if 0      
      whiteice::math::vertex< whiteice::math::blas_real<double> > x0;

      if(net->exportdata(x0) == false){
	delete net;
	return false;
      }

      // create dataset<>
      whiteice::dataset< whiteice::math::blas_real<double> > ds;
      if(ds.createCluster("input", 3*picsize*picsize)  == false ||
	 ds.createCluster("output", 3*picsize*picsize) == false){
	delete net;
	return false;
      }

      if(ds.add(0, data) == false || ds.add(1, data) == false){
	delete net;
	return false;
      }

      
      // NO LBFGS or optimization using gradients

      
      whiteice::LBFGS_nnetwork< whiteice::math::blas_real<double> > optimizer(*net, ds, false);

      if(optimizer.minimize(x0) == false){
	delete net;
	return false;
      }
      
      whiteice::math::vertex < whiteice::math::blas_real<double> > x;
      whiteice::math::blas_real<double> y;
      unsigned int iterations = 0;
      
      while(optimizer.isRunning() == true && optimizer.solutionConverged() == false){
      // while(iterations < 100){ // hard limiter to 1000 iterations/tries
	usleep(100);
	
	unsigned int current = 0;
	if(optimizer.getSolution(x, y, current)){

	  if(iterations != current){
	    iterations = current;
	    printf("AUTOENCODER OPTIMIZER %d: %f\n", iterations, y.c[0]);
	    fflush(stdout);
	  }
	}	
      }
#endif

      // after we have autoencoder nnetwork split it into encoder and decoder parts
      // 
      // HACK:
      // because last layer of dinrhiw's nnetwork implementation is always linear and
      // encoder's last layer shoud be sigmoidal, I add one extra identity function
      // layer to encoder

      // encoder: picture to feature vector (coded vector) [codes pictures as feature vector]
      // decoder: feature vector to picture [synthesizer]
      
      std::vector<unsigned int>  arch2;
      std::vector<unsigned int>& encoderArch = arch;
      std::vector<unsigned int>  decoderArch;
      
      net->getArchitecture(arch2);

      // last layer of encoder has same size as input layer of decoder
      decoderArch.push_back(encoderArch[encoderArch.size()-1]);

      // the rest is same as in encoder-decoder nnetwork..
      for(unsigned int l=encoderArch.size();l<arch2.size();l++)
	decoderArch.push_back(arch2[l]);

      // generates nnetworks
      encoder = new whiteice::nnetwork< whiteice::math::blas_real<double> > (encoderArch);
      decoder = new whiteice::nnetwork< whiteice::math::blas_real<double> > (decoderArch);

      if(encoder == nullptr || decoder == nullptr){
	delete net;
	if(encoder) delete encoder;
	if(decoder) delete decoder;
	return false;
      }

      try{
	
	for(unsigned int i=0;i<(encoderArch.size()-1);i++){
	  whiteice::math::matrix< whiteice::math::blas_real<double> > W;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > b;
	  if(net->getBias(b, i) == false) throw i;
	  if(net->getWeights(W, i) == false) throw i;
	  if(encoder->setBias(b, i) == false) throw i;
	  if(encoder->setWeights(W, i) == false) throw i;
	  if(encoder->setNonlinearity
	     (i, net->getNonlinearity(i)) == false) throw i;
	}

	const int L = encoderArch.size()-1; // continue from L:th layer
	
	for(unsigned int i=0;i<(decoderArch.size()-1);i++){
	  whiteice::math::matrix< whiteice::math::blas_real<double> > W;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > b;
	  if(net->getBias(b, L+i) == false) throw (L+i);
	  if(net->getWeights(W, L+i) == false) throw (L+i);
	  if(decoder->setBias(b, i) == false)  throw (L+i);
	  if(decoder->setWeights(W, i) == false) throw (L+i);
	  if(decoder->setNonlinearity
	     (i, net->getNonlinearity(L+i)) == false) throw (L+i);
	}
	
      }
      catch(unsigned int i){
	printf("ERROR: setting bias/weight for layer %d (%d %d %d) failed\n",
	       i, arch2.size(), encoderArch.size(), decoderArch.size());

	delete net;
	delete encoder;
	delete decoder;

	encoder = nullptr;
	decoder = nullptr;

	return false;
      }

      // HACK recreate encoder so that the last layer is linear identity layer
      //      and outputs of encoder are proper sigmoids
      {
	std::vector<unsigned int>  arch3;
	
	encoder->getArchitecture(arch3);
	const unsigned int dim = arch3[arch3.size()-1];
	
	arch3.push_back(dim);
	auto newEncoder = new whiteice::nnetwork< whiteice::math::blas_real<double> >(arch3);
	
	try {
	  for(unsigned int l=0;l<encoder->getLayers();l++){
	    whiteice::math::matrix< whiteice::math::blas_real<double> > W;
	    whiteice::math::vertex< whiteice::math::blas_real<double> > b;
	    if(encoder->getWeights(W, l) == false) throw l;
	    if(encoder->getBias(b, l) == false) throw l;
	    if(newEncoder->setWeights(W, l) == false) throw l;
	    if(newEncoder->setBias(b, l) == false) throw l;
	    if(newEncoder->setNonlinearity
	       (l, encoder->getNonlinearity(l)) == false) throw l;
	  }
	  
	  // additional extra layer (identity layer)
	  
	  whiteice::math::matrix< whiteice::math::blas_real<double> > W;
	  whiteice::math::vertex< whiteice::math::blas_real<double> > b;
	  
	  W.resize(dim, dim);
	  b.resize(dim);
	  
	  W.identity();
	  b.zero();
	  
	  if(newEncoder->setWeights(W, encoder->getLayers()) == false) throw encoder->getLayers();
	  if(newEncoder->setBias(b, encoder->getLayers()) == false) throw encoder->getLayers();
	  newEncoder->setNonlinearity
	    (encoder->getLayers(),
	     whiteice::nnetwork< whiteice::math::blas_real<double> >::pureLinear);
	  
	  delete encoder;
	  encoder = newEncoder;
	}
	catch(unsigned int l){
	  delete newEncoder;
	  delete encoder;
	  delete decoder;
	  delete net;
	  
	  printf("ERROR: adding extra layer to encoder failed: %d\n", l);
	  
	  return false;
	}
      }


      // dbn imported networks to have stochastic non-linearities
      encoder->setNonlinearity(whiteice::nnetwork< whiteice::math::blas_real<double> >::stochasticSigmoid);
      decoder->setNonlinearity(whiteice::nnetwork< whiteice::math::blas_real<double> >::stochasticSigmoid);

      // testing: tests output from DBN and nnetwork are the same
      for(unsigned int k=0;k<data.size();k++)
      {
	whiteice::math::vertex< whiteice::math::blas_real<double> >& d = data[k];
	whiteice::math::vertex< whiteice::math::blas_real<double> > out1;
	whiteice::math::vertex< whiteice::math::blas_real<double> > out2;

	if(dbn.setVisible(d) == false){
	  std::cout << "ERROR: cannot set DBN visible vector" << std::endl;
	}

	if(dbn.reconstructData(2) == false){
	  std::cout << "ERROR: cannot calculate DBN hidden vector" << std::endl;
	}

	out1 = dbn.getHidden();
	
	if(encoder->calculate(d, out2) == false){
	  std::cout << "ERROR: cannot encode picture to feature vector (nnetwork)" << std::endl;
	}

	std::cout << "DBN OUT : " << out1 << std::endl;
	std::cout << "NET OUT : " << out2 << std::endl;
      }

      
      return true;
    }
    
    
    
    void generateRandomPictures(whiteice::dataset< whiteice::math::blas_real<double> >& preprocess,
				whiteice::nnetwork< whiteice::math::blas_real<double> >*& decoder)
				
    {
      // open window (SDL)

      SDL_Window* window = NULL;
      
      int W = 640;
      int H = 480;
      
      
      SDL_DisplayMode mode;
      
      if(SDL_GetCurrentDisplayMode(0, &mode) == 0){
	W = (3*mode.w)/4;
	H = (3*mode.h)/4;
      }

      window = SDL_CreateWindow("Renaissance",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				W, H,
				SDL_WINDOW_ALWAYS_ON_TOP |
				SDL_WINDOW_INPUT_FOCUS);
      SDL_RaiseWindow(window);
      SDL_UpdateWindowSurface(window);
      SDL_RaiseWindow(window);

      SDL_Event event;
      bool exit = false;

      whiteice::math::vertex< whiteice::math::blas_real<double> > v;
      whiteice::math::vertex< whiteice::math::blas_real<double> > input;
      input.resize(decoder->input_size());
      v.resize(decoder->output_size());
      input.zero();

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

	// flips a single bit to be different in input
	const unsigned int index = rand()%input.size();
	input[index] = (input[index] > 0.5) ? 0.0 : 1.0;

	
	decoder->calculate(input, v);
	preprocess.invpreprocess(0, v); // removes preprocessing for the decoder
	
	// std::cout << input << std::endl;
	// std::cout << v << std::endl;

	const int picsize = (int)(sqrt(v.size()/3.0));
	
	if(vectorToSurface(v, picsize, scaled) == false)
	  continue;
	

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

	// SDL_BlitSurface(scaled, NULL, win, &imageRect);
	SDL_BlitScaled(scaled, NULL, win, &imageRect);

	SDL_FreeSurface(scaled);

	SDL_UpdateWindowSurface(window);
	SDL_ShowWindow(window);
	SDL_FreeSurface(win);

	sleep(1);
      }

      
      SDL_DestroyWindow(window);
    }
    
  }
}
