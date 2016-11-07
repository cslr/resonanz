
#include "optimizeResponse.h"

namespace whiteice
{
  namespace resonanz
  {
    
    bool optimizeResponse(whiteice::nnetwork< whiteice::math::blas_real<double> >*& nn,
			  const whiteice::dataset< whiteice::math::blas_real<double> >& data)
    {
      if(data.getNumberOfClusters() != 3 || nn != nullptr)
	return false; // bad parameters

      if(data.size(0) != data.size(1) || data.size(1) != data.size(2))
	return false;

      if(data.size(0) <= 0)
	return false;

      // transforms data to own internal dataset (bad coding)
      whiteice::dataset< whiteice::math::blas_real<double> > ds;

      if(ds.createCluster("input", data.dimension(0)+data.dimension(1)) == false) return false;
      if(ds.createCluster("output", data.dimension(2)) == false) return false;

      for(unsigned int i=0;i<data.size(0);i++){
	whiteice::math::vertex< whiteice::math::blas_real<double> > in1 = data.access(0, i);
	whiteice::math::vertex< whiteice::math::blas_real<double> > in2 = data.access(1, i);
	whiteice::math::vertex< whiteice::math::blas_real<double> > out = data.access(2, i);

	whiteice::math::vertex< whiteice::math::blas_real<double> > in(in1.size()+in2.size());

	if(in.write_subvertex(in1, 0) == false) return false;
	if(in.write_subvertex(in2, in1.size()) == false) return false;
	
	if(ds.add(0, in) == false) return false;
	if(ds.add(1, out) == false) return false;
      }

      //////////////////////////////////////////////////////////////////////
      // now our data is in ds dataset in a correct format for learning

      std::vector<unsigned int> arch;
      whiteice::math::vertex< whiteice::math::blas_real<double> > x0;

      arch.push_back(ds.dimension(0));
      arch.push_back(100*ds.dimension(0)); // use very wide nnetwork (can approximate any function)
      arch.push_back(ds.dimension(1));
      
      nn = new whiteice::nnetwork< whiteice::math::blas_real<double> > (arch);

      if(nn->exportdata(x0) == false){
	delete nn;
	return false;
      }

      whiteice::LBFGS_nnetwork< whiteice::math::blas_real<double> > optimizer(*nn, ds, false);
      
      if(optimizer.minimize(x0) == false){
	delete nn;
	return false;
      }
      
      whiteice::math::vertex < whiteice::math::blas_real<double> > x;
      whiteice::math::blas_real<double> y;
      unsigned int iterations = 0;

      while(optimizer.isRunning() == true && optimizer.solutionConverged() == false){
	usleep(100000); // 100ms sleep time
	
	unsigned int current = 0;
	if(optimizer.getSolution(x, y, current)){

	  if(iterations != current){
	    iterations = current;
	    printf("NN state prediction optimizer %d: %f\n", iterations, y.c[0]);
	    fflush(stdout);
	  }
	}
      }

      return true;
    }
    
    
  };
};
