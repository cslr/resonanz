

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dinrhiw/dinrhiw.h>

#include "SDLCartPole.h"


int main(int argc, char** argv)
{
  printf("CART POLE VISUALIZATION\n");

  srand(time(0));

  if(argc <= 1){
    whiteice::SDLCartPole< whiteice::math::blas_real<double> > system;

    system.setEpsilon(0.90); // 90% of examples are selected accoring to model
    system.setLearningMode(true);
    
    system.load("rifl.dat");
    
    system.start();

    while(system.isRunning()){
      sleep(180); // saved model file every 3 minutes
      system.save("rifl.dat");
      printf("MODEL FILE SAVED\n");
    }
    
    system.stop();
    
  }
  else if(strcmp(argv[1], "use") == 0){

    whiteice::SDLCartPole< whiteice::math::blas_real<double> > system;

    system.setEpsilon(1.00); // 100% of examples are selected accoring to model
    system.setLearningMode(false);
    
    if(system.load("rifl.dat") == false){
      printf("ERROR: loading model file failed.\n");
      return -1;
    }
    
    system.start();

    while(system.isRunning());
    
    system.stop();
  }
  
}
