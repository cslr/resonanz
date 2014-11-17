/*
 * SinusoidGenerator.cpp
 *
 *  Created on: 1.3.2013
 *      Author: omistaja
 */

#include "SinusoidGenerator.h"
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "timing.h"


void* __sinusoid_generator_thread(void* ptr);


SinusoidGenerator::SinusoidGenerator()
{
	current_value = 0.0f;
	keep_generating = true;

	assert(pthread_create(&genn_thread, NULL, __sinusoid_generator_thread, (void*)this) == 0);
}

SinusoidGenerator::~SinusoidGenerator()
{
	keep_generating = false;
	pthread_kill(genn_thread, SIGTERM);
}


bool SinusoidGenerator::data(std::vector<float>& x) const
{
	x.resize(NUMBER_OF_SIGNALS);

	for(unsigned int i=0;i<NUMBER_OF_SIGNALS;i++){
		x[i] = current_value;
	}

	return true;
}


unsigned int SinusoidGenerator::getNumberOfSignals() const
{
	return NUMBER_OF_SIGNALS;
}


double SinusoidGenerator::current_time()
{
	struct timeval tv;
	struct timezone tz;

	if(gettimeofday(&tv, &tz) == 0){
		double t = ((double)tv.tv_sec);
		t += ((double)tv.tv_usec)/1000000.0;

		return t;
	}
	else return 0.0f;
}


void SinusoidGenerator::generate()
{
	int update_freq = 20;
	double f = 0.5;

	double t0 = current_time();

	while(keep_generating){
		microsleep(1000/update_freq);
		double t1 = current_time();

		current_value = (float)(0.5*(sin(f*(t1-t0)) + 1.0));
	}
}


void* __sinusoid_generator_thread(void* ptr)
{
	if(ptr == NULL) return NULL;

	SinusoidGenerator* p = (SinusoidGenerator*)ptr;

	p->generate();

	return NULL;
}
