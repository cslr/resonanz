/*
 * EmotivInsightAffectiv.cpp
 *
 *  Created on: 13.6.2015
 *      Author: Tomas
 */

#include "EmotivInsightAffectiv.h"

#include <chrono>
#include <thread>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <vector>
#include <sys/time.h>
#include <stdexcept>
#include "timing.h"

namespace whiteice {
namespace resonanz {

// currently assumes only single user and ignores UserID field [multiple different EEG devices connected to same system]


EmotivInsightAffectiv::EmotivInsightAffectiv()
{
	__class_creation_time = std::chrono::high_resolution_clock::now();

#if HAS_EMOTIV
	// connection = IEE_EngineConnect();
	connection = IEE_EngineRemoteConnect("127.0.0.1", 1726);

	eEvent = IEE_EmoEngineEventCreate();
	eState       = IEE_EmoStateCreate();
#endif

	keep_polling = true;

	buffer_min_time = +100000000.0f;
	buffer_max_time = -100000000.0f;

	latest_value.resize(NUMBER_OF_SIGNALS);
	latest_variance.resize(NUMBER_OF_SIGNALS);

	for(unsigned int i=0;i<latest_value.size();i++){
		latest_value[i] = 0.5f;
		latest_variance[i] = 1.0f;
	}

	latest_data_received_t = 0;

	try{
		poll_thread = nullptr;
		poll_thread = new std::thread(poll_events, this);
		poll_thread->detach();
	}
	catch(std::exception& e){
#if HAS_EMOTIV
		if(connection == EDK_OK) IEE_EngineDisconnect();
		IEE_EmoStateFree(eState);
		IEE_EmoEngineEventFree(eEvent);
#endif

		poll_thread = nullptr;

		throw std::runtime_error("Cannot generate emotiv data polling thread.");
	}
}

EmotivInsightAffectiv::~EmotivInsightAffectiv()
{
	{
		std::lock_guard<std::mutex> lock(emotiv_lock);

		keep_polling = false;
		if(poll_thread != nullptr) delete poll_thread;
		poll_thread = nullptr;

#if HAS_EMOTIV
		if(connection == EDK_OK) IEE_EngineDisconnect();
		IEE_EmoStateFree(eState);
		IEE_EmoEngineEventFree(eEvent);
#endif
	}
}

/**
 * Returns true if connection and data collection to device is currently working.
 */
bool EmotivInsightAffectiv::connectionOk() const
{
	double t = (current_time() - latest_data_received_t) / time_frequency;

#if HAS_EMOTIV
	// connection is ok if Emotive Emo engine connection is ok and latest data from device is at most 2.0 seconds old
	return (connection == EDK_OK) && (t <= 2.0);
#else
	return (t <= 2.0);
#endif
}

/**
 * returns vectors of data starting from t0 ds (100ms) since start of the data collection
 * returns false if there is no data or if t0 > current time.
 *
 * data is in format x[t][signal number];
 */
bool EmotivInsightAffectiv::data(std::vector< std::vector<float> >& x, int t0, int t1) const
{
	if(t0 < 0) t0 = values.size() + t0;
	if(t0 < 0) return false; // too large negative index
	if(t0 >= (int)values.size()) return false;

	if(t1 <= 0) t1 = values.size() + t1;
	if(t1 < 0) return false;
	if(t1 > (int)values.size()) return false;

	if(t1 < t0) return false; // negative length vector

	{
		std::lock_guard<std::mutex> lock(data_lock);

		x.resize(t1 - t0);
		for(int t=t0;t<t1;t++)
			x[t-t0] = values[t];
	}

	return true;
}

/**
 * returns most recent data vector
 */
bool EmotivInsightAffectiv::data(std::vector<float>& x) const
{
	std::lock_guard<std::mutex> lock(data_lock);

	x = latest_value;

	return true;
}

/**
 * returns estimated variance of measured data.
 */
bool EmotivInsightAffectiv::variance(std::vector< std::vector<float> >& v, int t0, int t1) const
{
	if(t0 < 0) t0 = variances.size() + t0;
	if(t0 < 0) return false; // too large negative index
	if(t0 >= (int)variances.size()) return false;

	if(t1 <= 0) t1 = variances.size() + t1;
	if(t1 < 0) return false;
	if(t1 > (int)variances.size()) return false;

	if(t1 < t0) return false; // negative length vector

	{
		std::lock_guard<std::mutex> lock(data_lock);

		v.resize(t1 - t0);
		for(int t=t0;t<t1;t++)
			v[t-t0] = variances[t];
	}

	return true;
}

/**
 * returns variance vector for most recent measurements
 */
bool EmotivInsightAffectiv::variance(std::vector<float>& v) const
{
	std::lock_guard<std::mutex> lock(data_lock);

	v = latest_variance;

	return true;
}


/**
 * return number of 1d-signals in data
 */
unsigned int EmotivInsightAffectiv::getNumberOfSignals() const
{
	return NUMBER_OF_SIGNALS;
}

/**
 * return signal name for the given signal number
 */
std::string EmotivInsightAffectiv::getSignalName(int index) const
{
	switch(index){
	case 0: return "Insight: Excitement";
	case 1: return "Insight: Relaxation";
	case 2: return "Insight: Stress";
	case 3: return "Insight: Engagement";
	case 4: return "Insight: Interest";
	default:
		return "N/A";
	};

	return "N/A";
}


bool EmotivInsightAffectiv::getSignalNames(std::vector<std::string>& names) const
{
	names.resize(5);

	names[0] = "Insight: Excitement";
	names[1] = "Insight: Relaxation";
	names[2] = "Insight: Stress";
	names[3] = "Insight: Engagement";
	names[4] = "Insight: Interest";

	return true;
}

/**
 * Returns amount of data collected [collection frequency is 10 Hz]
 */
int EmotivInsightAffectiv::getDataLength() const
{
	std::lock_guard<std::mutex> lock(data_lock);

	return values.size();
}


bool EmotivInsightAffectiv::clearData()
{
	std::lock_guard<std::mutex> lock(data_lock);

	values.clear();
	variances.clear();

	return true;
}


double EmotivInsightAffectiv::current_time() const
{
	std::chrono::high_resolution_clock::time_point tnow =
			std::chrono::high_resolution_clock::now();

	typedef std::chrono::duration<double> fsec;

	fsec delta_in_seconds = tnow - __class_creation_time;

	return time_frequency*delta_in_seconds.count();
}


///////////////////////////////////////////////////////////////
// EEG data polling and processing

// generates data from observations and samples it to the time-line
void EmotivInsightAffectiv::update_tick(double t0, double t1)
{
	if(t1 <= t0) return; // nothing to do
	unsigned int delta = (unsigned int)(t1 - t0);
	if(delta <= 0) return; // nothing to do

	//////////////////////////////////////////////////////////////
	// process data

	std::lock_guard<std::mutex> lock(data_lock);

	if(times.size() == 0){
		for(unsigned int i=0;i<delta;i++){
			values.push_back(latest_value);
			variances.push_back(latest_variance);
		}
	}
	else if(times.size() == 1){
		std::vector<float> variance;
		variance.resize(NUMBER_OF_SIGNALS);

		for(unsigned int i=0;i<variance.size();i++)
			variance[i] = 1.0f;

		for(unsigned int i=0;i<delta;i++){
			values.push_back(samples[0]);
			variances.push_back(variance);
		}

		latest_value    = samples[0];
		latest_variance = variance;
	}
	else{
		if(delta == 1){ // single time step
			std::vector<float> mean;
			std::vector<float> variance;

			mean.resize(NUMBER_OF_SIGNALS);
			variance.resize(NUMBER_OF_SIGNALS);

			for(unsigned int i=0;i<mean.size();i++){
				mean[i] = 0.0f;
				variance[i] = 0.0f;
			}

			for(unsigned int i=0;i<samples.size();i++){
				for(unsigned int j=0;j<mean.size();j++){
					mean[j] += samples[i][j];
					variance[j] += samples[i][j]*samples[i][j];
				}
			}

			for(unsigned int j=0;j<mean.size();j++){
				mean[j] /= ((float)samples.size());
				variance[j] /= ((float)samples.size());

				variance[j] = variance[j] - mean[j]*mean[j];
			}

			values.push_back(mean);
			variances.push_back(variance);

			latest_value    = mean;
			latest_variance = variance;
		}
		else{
			// do some interpolation over multiple delta values

			float time_delta = (buffer_max_time - buffer_min_time)/delta;

			for(unsigned int i=0;i<delta;i++){
				float tt0 = buffer_min_time + i*time_delta;
				float tt1 = buffer_min_time + (i+1)*time_delta;

				// search for value indexes for given delta value in observations
				unsigned int index0 = 0;
				unsigned int index1 = times.size();

				for(unsigned int j=0;j<times.size();j++)
					if(times[j] >= tt0){ index0 = j; break; }

				for(unsigned int j=0;j<times.size();j++)
					if(times[j] >= tt1){ index1 = j; break; }

				if(index0 >= times.size()) index0 = times.size() - 1;
				if(index1 > times.size()) index1 = times.size();
				if(index0 == index1){
					if(index1 < times.size()) index1++;
					else if(index0 > 0) index0--;
					else{
						index0 = 0;
						index1 = times.size();
					}
				}

				std::vector<float> mean;
				std::vector<float> variance;
				mean.resize(NUMBER_OF_SIGNALS);
				variance.resize(NUMBER_OF_SIGNALS);

				for(unsigned int j=0;j<NUMBER_OF_SIGNALS;j++){
					mean[j] = 0.0f;
					variance[j] = 0.0f;
				}

				for(unsigned index=index0;index<index1;index++){
					for(unsigned int j=0;j<NUMBER_OF_SIGNALS;j++){
						mean[j] += samples[i][j];
						variance[j] += samples[i][j]*samples[i][j];
					}
				}

				unsigned int ssize = (index1 - index0);

				for(unsigned int j=0;j<mean.size();j++){
					mean[j] /= ((float)ssize);
					variance[j] /= ((float)ssize);

					if(ssize > 1){
						variance[j] = variance[j] - mean[j]*mean[j];
					}
					else variance[j] = 1.0f; // large value, cannot measure for a single observation
				}

				values.push_back(mean);
				variances.push_back(variance);

				latest_value    = mean;
				latest_variance = variance;
			}

		}
	}

	//////////////////////////////////////////////////////////////

	// reset
	times.clear();
	samples.clear();
	buffer_min_time = +100000000.0f;
	buffer_max_time = -100000000.0f;
}

// handles new data entry
void EmotivInsightAffectiv::new_data(float t, std::vector<float>& data)
{
	// debugging messages for now
	/*
	printf("DATA (%f): ", t);
	for(unsigned int i=0;i<data.size();i++)
		printf("%f ", data[i]);
	printf("\n");
	fflush(stdout);
	*/

	if(times.size() >= MAX_BUFFER_SIZE) return; // do nothing

	times.push_back(t);
	samples.push_back(data);

	if(t < buffer_min_time) buffer_min_time = t;
	if(t > buffer_max_time) buffer_max_time = t;
}


void EmotivInsightAffectiv::poll_events()
{
	int poll_freq = 20; // Hz

	double t0 = current_time();

	latest_data_received_t = current_time();

	while(keep_polling){

#if HAS_EMOTIV
		// 1. check that there is connection
		while(connection != EDK_OK){
			{
				std::lock_guard<std::mutex> lock(emotiv_lock);
				// connection = IEE_EngineRemoteConnect("127.0.0.1", 1726);
				connection = IEE_EngineConnect();
			}

			microsleep(1000/poll_freq);

			double t1 = current_time();
			if(t1 - t0 >= 1.0){ update_tick(t0, t1); t0 = t1; }
		}


		// 2. once we have connection, poll for events
		// TODO: how to notice if the connection goes down??
		emotiv_lock.lock();

		int state = IEE_EngineGetNextEvent(eEvent);

		while(state == EDK_OK){
/*
			printf("GOT EMOTIV EVENT\n");
			fflush(stdout);
*/
			IEE_Event_t event_type = IEE_EmoEngineEventGetType(eEvent);
			// EE_EmoEngineEventGetUserId(eEvent, &userID);

			if(event_type == IEE_EmoStateUpdated){
				IEE_EmoEngineEventGetEmoState(eEvent, eState);

				// get affectiv scores as they are most usable
				std::vector<float> scores;

				float t = IS_GetTimeFromStart(eState);

				if(IS_PerformanceMetricIsActive(eState, PM_EXCITEMENT))
					scores.push_back(IS_PerformanceMetricGetInstantaneousExcitementScore(eState));
				else
					scores.push_back(0.5f);

				// ignore ES_AffectivGetExcitementLongTermScore()

				if(IS_PerformanceMetricIsActive(eState, PM_RELAXATION))
					scores.push_back(IS_PerformanceMetricGetRelaxationScore(eState));
				else scores.push_back(0.5f);

				if(IS_PerformanceMetricIsActive(eState, PM_STRESS))
					scores.push_back(IS_PerformanceMetricGetStressScore(eState));
				else scores.push_back(0.5f);

				if(IS_PerformanceMetricIsActive(eState, PM_ENGAGEMENT))
					scores.push_back(IS_PerformanceMetricGetEngagementBoredomScore(eState));
				else scores.push_back(0.5f);

				if(IS_PerformanceMetricIsActive(eState, PM_INTEREST))
					scores.push_back(IS_PerformanceMetricGetInterestScore(eState));
				else
					scores.push_back(0.5f);
/*
				for(unsigned int i=0;i<scores.size();i++)
					printf("%.2f ", scores[i]);
				printf("\n");
				fflush(stdout);
*/
				/////////////////////////////////////////////////////////////////
				// process collected data here [add to datastream we are maintaining]

				latest_data_received_t = current_time();

				new_data(t, scores);
			}

			state = IEE_EngineGetNextEvent(eEvent);
		}

		if(state == EDK_EMOENGINE_DISCONNECTED){
			connection = EDK_OK+1; // must re-connect
		}

		emotiv_lock.unlock();
#endif

		microsleep(1000/poll_freq);

		double t1 = current_time();
		if(t1 - t0 >= 1.0){ update_tick(t0, t1); t0 = t1; }
	}

#if HAS_EMOTIV
	IEE_EngineDisconnect();
	connection = EDK_OK + 1;
#endif

}





} /* namespace resonanz */
} /* namespace whiteice */

