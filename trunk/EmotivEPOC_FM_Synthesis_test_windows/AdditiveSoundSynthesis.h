/*
 * AdditiveSoundSynthesis.h
 *
 *  Created on: 15.2.2013
 *      Author: omistaja
 */

#ifndef ADDITIVESOUNDSYNTHESIS_H_
#define ADDITIVESOUNDSYNTHESIS_H_

#include "SDLSoundSynthesis.h"

class AdditiveSoundSynthesis: public SDLSoundSynthesis {
public:
	AdditiveSoundSynthesis();
	virtual ~AdditiveSoundSynthesis();

	virtual bool reset();

	virtual bool setParameters(std::vector<float>& p);

	virtual int getNumberOfParameters();

protected:

	virtual bool synthesize(int16_t* buffer, int samples);

	static const int NUM_SINUSOIDS = 10;

	double tbase;

	std::vector<float> f;
	std::vector<float> r;
	std::vector<float> p;

};

#endif /* ADDITIVESOUNDSYNTHESIS_H_ */
