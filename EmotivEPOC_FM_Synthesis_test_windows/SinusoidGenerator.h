/*
 * SinusoidGenerator.h
 *
 *  Created on: 1.3.2013
 *      Author: omistaja
 */

#ifndef SINUSOIDGENERATOR_H_
#define SINUSOIDGENERATOR_H_

#include <vector>
#include <pthread.h>
#include "DataSource.h"

class SinusoidGenerator : public DataSource {
public:
	SinusoidGenerator();
	virtual ~SinusoidGenerator();

	/**
	 * returns current value
	 */
	bool data(std::vector<float>& x) const;

	unsigned int getNumberOfSignals() const;

protected:

	double current_time();

private:
	float current_value;
	static const unsigned int NUMBER_OF_SIGNALS = 4;

	void generate();

	bool keep_generating;
	pthread_t genn_thread;

	friend void* __sinusoid_generator_thread(void*);
};

#endif /* SINUSOIDGENERATOR_H_ */
