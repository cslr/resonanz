/*
 * RandomEEG.cpp
 *
 *  Created on: 2.7.2015
 *      Author: Tomas
 */

#include "RandomEEG.h"
#include <stdio.h>
#include <stdlib.h>


namespace whiteice {
namespace resonanz {

RandomEEG::RandomEEG() {

}

RandomEEG::~RandomEEG() {

}


/**
 * Returns true if connection and data collection to device is currently working.
 */
bool RandomEEG::connectionOk() const
{
	return true; // always connected
}


/**
 * returns current value
 */
bool RandomEEG::data(std::vector<float>& x) const
{
	x.resize(10);

	x[0] = rand() / ((double)RAND_MAX);
	x[1] = rand() / ((double)RAND_MAX);
	x[2] = rand() / ((double)RAND_MAX);
	x[3] = rand() / ((double)RAND_MAX);
	x[4] = rand() / ((double)RAND_MAX);
	x[5] = rand() / ((double)RAND_MAX);
	x[6] = rand() / ((double)RAND_MAX);
	x[7] = rand() / ((double)RAND_MAX);
	x[8] = rand() / ((double)RAND_MAX);
	x[9] = rand() / ((double)RAND_MAX);

	return true;
}


bool RandomEEG::getSignalNames(std::vector<std::string>& names) const
{
	names.resize(10);

	names[0] = "Random EEG 1";
	names[1] = "Random EEG 2";
	names[2] = "Random EEG 3";
	names[3] = "Random EEG 4";
	names[4] = "Random EEG 5";
	names[5] = "Random EEG 6";
	names[6] = "Random EEG 7";
	names[7] = "Random EEG 8";
	names[8] = "Random EEG 9";
	names[9] = "Random EEG 10";

	return true;
}


unsigned int RandomEEG::getNumberOfSignals() const
{
	return 10;
}



} /* namespace resonanz */
} /* namespace whiteice */
