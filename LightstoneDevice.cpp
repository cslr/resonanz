/*
 * LightStoneDevice.cpp
 *
 *  Created on: 4.7.2015
 *      Author: Tomas
 */

#include "LightStoneDevice.h"
#include "lightstone/lightstone.h"
#include <stdio.h>

#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include <math.h>
#include <unistd.h>

#include "Log.h"



namespace whiteice {
namespace resonanz {

LightstoneDevice::LightstoneDevice() {
	// starts a thread that does all the hard work

	running = true;
	has_lightstone = false;
	latest_data_point_added = 0;
	worker = new std::thread(lightstone_loop, this);
}

LightstoneDevice::~LightstoneDevice() {

	std::lock_guard<std::mutex> lock(start_mutex);
	// in theory there can be multiple threads calling same destructor..

	running = false;
	worker->join();
	delete worker;
}


/*
 * Returns unique DataSource name
 */
std::string LightstoneDevice::getDataSourceName() const
{
	return "WD Lightstone";
}

/**
 * Returns true if connection and data collection to device is currently working.
 */
bool LightstoneDevice::connectionOk() const
{
	// returns true if device is initialized and connected and
	// time from the latest data point is less than 2 seconds

	if(running == false || has_lightstone == false) return false;

	auto duration1 = std::chrono::system_clock::now().time_since_epoch();
	auto current_time =
			std::chrono::duration_cast<std::chrono::milliseconds>(duration1).count();

	if(current_time - latest_data_point_added < 2000)
		return true;
	else
		return false;

}

/**
 * returns current value
 */
bool LightstoneDevice::data(std::vector<float>& x) const
{
	std::lock_guard<std::mutex> lock(data_mutex);

	if(latest_data_point_added > 0){
		x = this->value;
		return true;
	}
	else{
		x.resize(2);
		return false;
	}
}

bool LightstoneDevice::getSignalNames(std::vector<std::string>& names) const
{
	names.resize(2);
	names[0] = "Lightstone: Heart Rate";
	names[1] = "Lightstone: Conductance";
	return true;
}

unsigned int LightstoneDevice::getNumberOfSignals() const
{
	return 2;
}


void LightstoneDevice::lightstone_loop()
{
	logging.info("lightstone data fetcher thread started");

	lightstone* usb = lightstone_create();

	while(running){
		int count = 0;
		if((count = lightstone_get_count(usb)) <= 0){
			sleep(1);
		}
		else{
			bool open = false;

			for(int i=0;i<count;i++)
				if(lightstone_open(usb, i) >= 0)
					open = true;

			if(open)
				break;
		}
	}

	logging.info("lightstone usb initialization done");

	has_lightstone = true;

	// starts collecting data after successful opening of the device

	auto duration0 = std::chrono::system_clock::now().time_since_epoch();
	auto t0 = std::chrono::duration_cast<std::chrono::milliseconds>(duration0).count();
	auto tstart = t0;

	std::vector<float> history;
	std::vector<float> t; // sampling times of the samples
	float sampling_hz = 15.0f;
	float previous_hr = 0.0f;
	float hr = 0.0f;

	while(running){
		auto r = lightstone_get_info(usb);

		auto duration1 = std::chrono::system_clock::now().time_since_epoch();
		auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(duration1).count();
		float dt = (t1 - t0)/1000.0f;
		t0 = t1;

		sampling_hz = 1/dt;
		auto s = r.hrv;

		history.push_back(s);
		t.push_back((t1-tstart)/1000.0f);


		while(history.size() > 3.0*sampling_hz){ // 3 second long history
			history.erase(history.begin());
			t.erase(t.begin());
		}

		hr = estimate_heart_rate(history, t, sampling_hz);

		if(hr == 0.0f) hr = previous_hr; // if we cannot estimate hr use previous good hr value
		previous_hr = hr;

		if(r.scl > 0.0f){
			// connection is ok and we have a new data point
			// printf("Current HR: %.2f bpm\n", hr);

			std::lock_guard<std::mutex> lock(data_mutex);
			std::vector<float> x(2);
			x[0] = hr;
			x[1] = r.scl;
			this->value = x;

			auto duration1 = std::chrono::system_clock::now().time_since_epoch();
			latest_data_point_added =
					std::chrono::duration_cast<std::chrono::milliseconds>(duration1).count();
		}


	}

	logging.info("lightstone thread: about shutdown");

	lightstone_delete(usb);
	has_lightstone = false;
}


// current heart rate from the data (for latest sample) from the data which is semi inregularly sampled at time points t
float LightstoneDevice::estimate_heart_rate(const std::vector<float>& data, const std::vector<float>& t, const float sampling_hz) const
{
	if(data.size() <= 2)
		return 0.0f; // cannot estimate heart rate with less than 2 samples

	std::vector<float> deriv(data.size()); // derivate

	for(unsigned int i=0;i<(deriv.size()-1);i++)
		deriv[i] = data[i+1] - data[i];

	std::vector<float> deltaT;
	int previous = -1;

	for(int i=0;i<(int)(deriv.size()-2);i++){
		if(deriv[i] >= 0.0f && deriv[i+1] <= 0.0f){ // derivate is zero
			// calculates mean and variance around this point
			float mean = 0.0f, var = 0.0f;
			int start = i+1-floor(sampling_hz/2.0f);
			int end   = i+1+floor(sampling_hz/2.0f);
			if(start < 0) start = 0;
			if(end > (signed)data.size()) end = data.size();

			calculate_mean_var(data, start, end, mean, var);

			const float cutoff = mean + sqrt(var);

			if(data[i+1] >= cutoff){
				if(previous >= 0){
					deltaT.push_back(t[(i+1)] - t[previous]);
				}
				previous = i+1;
			}
		}
	}

	// calculates median of distances
	if(deltaT.size() > 0){
		std::sort(deltaT.begin(), deltaT.end());

		float median = 0.0f;

		if(deltaT.size() > 1){
			float index = deltaT.size()/2.0f;
			median =
				(deltaT[(unsigned int)floor(index)]+deltaT[(unsigned int)ceil(index)])/2.0f;
		}
		else{
			median = deltaT[0];
		}

		// [s/min] * [samples/s] / [samples/beat] = [samples/min]*[beat/samples] = [beat/min]

		return (60.0f/median);
	}
	else{
		return 0.0f; // no heart rate detected..
	}
}


void LightstoneDevice::calculate_mean_var(const std::vector<float>& data,
		const unsigned int start, const unsigned int end, float& mean, float& var) const
{
	// calculates mean and variance of the data
	mean = 0.0f;
	var = 0.0f;
	float N = 0.0f;

	for(int i=(signed)start;i<(signed)end;i++){
		mean += data[i];
		var  += data[i]*data[i];
		N++;
	}

	mean /= N;
	var  /= N;

	var = (var - mean*mean)*(N/(N-1)); // sample variance..
}



} /* namespace resonanz */
} /* namespace whiteice */
