/*
 * EmotivInsightAffectiv.h
 *
 *  Created on: 13.6.2015
 *      Author: Tomas
 */

#ifndef EMOTIVINSIGHTAFFECTIV_H_
#define EMOTIVINSIGHTAFFECTIV_H_

#include "DataSource.h"
#include "Iedk.h"
#include <time.h>
// #include <pthread.h>

#include <string>
#include <chrono>
#include <thread>
#include <mutex>

namespace whiteice {
namespace resonanz {

/**
 * Class to return Emotiv Insight Affectiv values
 */
class EmotivInsightAffectiv: public DataSource {
public:
	EmotivInsightAffectiv();
	virtual ~EmotivInsightAffectiv();

	/**
	 * Returns true if connection and data collection to device is currently working.
	 */
	bool connectionOk() const;

	/**
	 * returns data between t0..(t1-1) samples (total of t1-t0 samples)
	 * returns false if there is no data or if t0 > current time.
	 *
	 * negative values t0 means taking data from the end of the buffer [more recent values]:
	 * x[size(x)-abs(t0) .. size(x)-abs(t1)]
	 *
	 * if t1 is negative data is retrieved all the way to the end of the buffer
	 *
	 * data is in format x[t][signal number];
	 */
	bool data(std::vector< std::vector<float> >& x, int t0 = 0, int t1 = 0) const;

	/**
	 * returns most recent data vector
	 */
	bool data(std::vector<float>& x) const;

	/**
	 * returns estimated variance of measured data.
	 */
	bool variance(std::vector< std::vector<float> >& v, int t0 = 0, int t1 = 0) const;

	/**
	 * returns variance vector for most recent measurements
	 */
	bool variance(std::vector<float>& v) const;

	/**
	 * return number of 1d-signals in data
	 */
	unsigned int getNumberOfSignals() const;

	/**
	 * return signal name for the given signal number
	 */
	std::string getSignalName(int index) const;


	/**
	 * return all signal names
	 */
	bool getSignalNames(std::vector<std::string>& names) const;


	/**
	 * Returns amount of data collected [collection frequency is 10 Hz]
	 */
	int getDataLength() const;

	/**
	 * Removes already collected data.
	 */
	bool clearData();

protected:
	double current_time() const;

	std::chrono::high_resolution_clock::time_point __class_creation_time;

	// collection frequency [10 Hz]
	static constexpr double time_frequency = 10.0; // (100ms) => aprox 12.8 samples high-quality affectiv samples per second (internal 2048)



	void update_tick(double t0, double t1); // update time progression and adds fresh data to be used

	std::vector<float> latest_value;
	std::vector<float> latest_variance;

	std::vector< std::vector<float> > values;
	std::vector< std::vector<float> > variances;

	mutable std::mutex data_lock;
	static const unsigned int NUMBER_OF_SIGNALS = 5;

	///////////////////////////////////////////////////////////////////////////////////////
	virtual void new_data(float t, std::vector<float>& data); // records new data since last update_tick() call

	std::vector<float> times;
	float buffer_min_time, buffer_max_time;
	std::vector< std::vector<float> > samples;
	static const unsigned int MAX_BUFFER_SIZE = 1000; // maximum number of samples to be stored after which we ignore the rest

	double latest_data_received_t;


	////////////////////////////////////////////////////////////////////////////////////////
	virtual void poll_events();

	int connection;
	EmoEngineEventHandle eEvent;
	EmoStateHandle eState;

	bool keep_polling;

	mutable std::mutex emotiv_lock;
	std::thread* poll_thread;
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* EMOTIVINSIGHTAFFECTIV_H_ */
