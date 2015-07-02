/*
 * NoEEGDevice.h
 *
 *  Created on: 2.7.2015
 *      Author: Tomas
 */

#ifndef NOEEGDEVICE_H_
#define NOEEGDEVICE_H_

#include "DataSource.h"

namespace whiteice {
namespace resonanz {

/**
 * Placeholder for empty EEG device producing no outputs.
 */
class NoEEGDevice: public DataSource {
public:
	NoEEGDevice();
	virtual ~NoEEGDevice();


	virtual std::string getDataSourceName() const { return "No EEG device"; };

	/**
	 * Returns true if connection and data collection to device is currently working.
	 */
	virtual bool connectionOk() const { return true; }

	/**
	 *	 returns current value
	 */
	virtual bool data(std::vector<float>& x) const {
		x.resize(1);
		x[0] = 0.5f;
		return true;
	}

	virtual bool getSignalNames(std::vector<std::string>& names) const {
		names.resize(1);
		names[0] = "Empty signal";
		return true;
	}

	virtual unsigned int getNumberOfSignals() const { return 1; }
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* NOEEGDEVICE_H_ */
