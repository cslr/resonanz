/*
 * RandomEEG.h
 *
 *  Created on: 2.7.2015
 *      Author: Tomas
 */

#ifndef RANDOMEEG_H_
#define RANDOMEEG_H_

#include "DataSource.h"

namespace whiteice {
namespace resonanz {

class RandomEEG: public DataSource {
public:
	RandomEEG();
	virtual ~RandomEEG();


	virtual std::string getDataSourceName() const { return "Random EEG device"; };

	/**
	 * Returns true if connection and data collection to device is currently working.
	 */
	virtual bool connectionOk() const;

	/**
	 * returns current value
	 */
	virtual bool data(std::vector<float>& x) const;

	virtual bool getSignalNames(std::vector<std::string>& names) const;

	virtual unsigned int getNumberOfSignals() const;
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* RANDOMEEG_H_ */
