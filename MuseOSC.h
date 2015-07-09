/*
 * MuseOSC.h
 *
 *  Created on: 7.7.2015
 *      Author: Tomas
 */

#ifndef MUSEOSC_H_
#define MUSEOSC_H_

#include "DataSource.h"

#include <vector>
#include <stdexcept>
#include <exception>
#include <thread>
#include <mutex>

namespace whiteice {
namespace resonanz {

/**
 * Receives Muse OSC data from given UDP localhost port
 * Currently outputs 6 values
 *
 * delta, theta, alpha, beta, gamma absolute values fitted to [0,1] interval
 * total power: sum delta+theta+alpha+beta+gamma
 *
 */
class MuseOSC: public DataSource {
public:
	MuseOSC(unsigned int port) throw(std::runtime_error);
	virtual ~MuseOSC();

	/*
	 * Returns unique DataSource name
	 */
	virtual std::string getDataSourceName() const;

	/**
	 * Returns true if connection and data collection to device is currently working.
	 */
	virtual	 bool connectionOk() const;

	/**
	 * returns current value
	 */
	virtual bool data(std::vector<float>& x) const;

	virtual bool getSignalNames(std::vector<std::string>& names) const;

	virtual unsigned int getNumberOfSignals() const;

private:
	const unsigned int port;

	void muse_loop(); // worker thread loop

	std::thread* worker_thread;
	bool running;

	bool hasConnection;
	float quality; // quality of connection [0,1]

	mutable std::mutex data_mutex;
	std::vector<float> value; // currently measured value
	long long latest_sample_seen_t; // time of the latest measured value

};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* MUSEOSC_H_ */
