/*
 * AdditiveSoundSynthesis.cpp
 *
 * 10-parameter additive synthesis
 *
 *  Created on: 15.2.2013
 *      Author: omistaja
 */

#include "AdditiveSoundSynthesis.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

AdditiveSoundSynthesis::AdditiveSoundSynthesis()
{
	tbase = 0.0;

	r.resize(NUM_SINUSOIDS);
	p.resize(NUM_SINUSOIDS);
	f.resize(NUM_SINUSOIDS);

	double sum = 0.0;

	// double f0 = ((float)rand()) / (float)RAND_MAX;

	for(int i=0;i<NUM_SINUSOIDS;i++){
		f[i] = ((double)rand())/((double)RAND_MAX)*5000.0;
		r[i] = ((double)rand())/((double)RAND_MAX);
		p[i] = ((double)rand())/((double)RAND_MAX);
		sum += r[i];

		printf("%f\n", f[i]);
	}

	for(int i=0;i<NUM_SINUSOIDS;i++)
		r[i] /= sum;

}

AdditiveSoundSynthesis::~AdditiveSoundSynthesis() {

}


bool AdditiveSoundSynthesis::reset()
{
	tbase = 0.0;
	return true;
}


bool AdditiveSoundSynthesis::setParameters(std::vector<float>& p)
{
	if(p.size() != (3*NUM_SINUSOIDS)){
		f.resize(NUM_SINUSOIDS);
		r.resize(NUM_SINUSOIDS);
		p.resize(NUM_SINUSOIDS);

		double sum = 0.0;

		for(int i=0;i<NUM_SINUSOIDS;i++){
			r[i] = fabs(p[i]);
			p[i] = p[1*NUM_SINUSOIDS + i];
			f[i] = fabs(p[2*NUM_SINUSOIDS + i]);

			sum += r[i];
		}

		for(int i=0;i<NUM_SINUSOIDS;i++)
			r[i] /= sum;

		return true;
	}
	else{ return false; }

	return false;
}


int AdditiveSoundSynthesis::getNumberOfParameters()
{
	return (3*NUM_SINUSOIDS);
}



bool AdditiveSoundSynthesis::synthesize(int16_t* buffer, int samples)
{
	double hz = snd.freq;

	for(int i=0;i<samples;i++){
		double t = tbase + ((double)i)/hz;

		double value = 0.0;

		for(int j=0;j<NUM_SINUSOIDS;j++){
			value += r[j]*sin(f[j]*t + p[j]);
		}

		if(value > 1.0) value = 1.0;
		else if(value < -1.0) value = -1.0;

		buffer[i] = (int16_t)( value*(0x7FFF - 1) );
	}

	tbase += ((double)samples)/hz;

	return true;
}

