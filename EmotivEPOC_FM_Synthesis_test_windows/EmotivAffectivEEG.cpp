/*
 * EmotivAffectivEEG.cpp
 *
 *  Created on: 6.2.2013
 *      Author: omistaja
 */

#include "EmotivAffectivEEG.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <vector>
#include <sys/time.h>
#include <stdexcept>
#include "timing.h"

// FIXME currently assumes only single user and ignores UserID field [multiple different EEG devices connected to same system]

void* __emotiv_start_poll_thread(void* ptr);


EmotivAffectivEEG::EmotivAffectivEEG() {

	connection = EE_EngineConnect();

	eEvent = EE_EmoEngineEventCreate();
	eState       = EE_EmoStateCreate();

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

	if(pthread_mutex_init(&emotiv_lock, NULL) != 0){
		if(connection == EDK_OK) EE_EngineDisconnect();
		EE_EmoStateFree(eState);
		EE_EmoEngineEventFree(eEvent);

		throw std::runtime_error("Cannot initialize emotiv mutex.");
	}

	if(pthread_mutex_init(&data_lock, NULL) != 0){
		if(connection == EDK_OK) EE_EngineDisconnect();
		EE_EmoStateFree(eState);
		EE_EmoEngineEventFree(eEvent);

		pthread_mutex_destroy(&emotiv_lock);

		throw std::runtime_error("Cannot initialize data mutex.");
	}

	if(pthread_create(&poll_thread, NULL, __emotiv_start_poll_thread, (void*)this) != 0){
		if(connection == EDK_OK) EE_EngineDisconnect();
		EE_EmoStateFree(eState);
		EE_EmoEngineEventFree(eEvent);

		pthread_mutex_destroy(&emotiv_lock);
		pthread_mutex_destroy(&data_lock);

		throw std::runtime_error("Cannot generate emotiv data polling thread.");
	}
}

EmotivAffectivEEG::~EmotivAffectivEEG() {

	pthread_mutex_lock(&emotiv_lock);

	keep_polling = false;
	pthread_kill(poll_thread, SIGTERM);

	if(connection == EDK_OK) EE_EngineDisconnect();
	EE_EmoStateFree(eState);
	EE_EmoEngineEventFree(eEvent);

	pthread_mutex_unlock(&emotiv_lock);

	pthread_mutex_destroy(&emotiv_lock);
	pthread_mutex_destroy(&data_lock);
}

/**
 * Returns true if connection and data collection to device is currently working.
 */
bool EmotivAffectivEEG::connectionOk() const
{
	double t = (current_time() - latest_data_received_t) / time_frequency;

	// connection is ok if Emotive Emo engine connection is ok and latest data from device is at most 2.0 seconds old
	return (connection == EDK_OK) && (t <= 2.0);
}

/**
 * returns vectors of data starting from t0 seconds since start of the data collection
 * returns false if there is no data or if t0 > current time.
 *
 * data is in format x[t][signal number];
 */
bool EmotivAffectivEEG::data(std::vector< std::vector<float> >& x, int t0, int t1) const
{
	if(t0 < 0) t0 = values.size() + t0;
	if(t0 < 0) return false; // too large negative index
	if(t0 >= (int)values.size()) return false;

	if(t1 <= 0) t1 = values.size() + t1;
	if(t1 < 0) return false;
	if(t1 > (int)values.size()) return false;

	if(t1 < t0) return false; // negative length vector

	pthread_mutex_lock(&data_lock);

	x.resize(t1 - t0);
	for(int t=t0;t<t1;t++){
		x[t-t0] = values[t];
	}

	pthread_mutex_unlock(&data_lock);

	return true;
}

/**
 * returns most recent data vector
 */
bool EmotivAffectivEEG::data(std::vector<float>& x) const
{
	pthread_mutex_lock(&data_lock);

	x = latest_value;

	pthread_mutex_unlock(&data_lock);

	return true;
}

/**
 * returns estimated variance of measured data.
 */
bool EmotivAffectivEEG::variance(std::vector< std::vector<float> >& v, int t0, int t1) const
{
	if(t0 < 0) t0 = variances.size() + t0;
	if(t0 < 0) return false; // too large negative index
	if(t0 >= (int)variances.size()) return false;

	if(t1 <= 0) t1 = variances.size() + t1;
	if(t1 < 0) return false;
	if(t1 > (int)variances.size()) return false;

	if(t1 < t0) return false; // negative length vector

	pthread_mutex_lock(&data_lock);

	v.resize(t1 - t0);
	for(int t=t0;t<t1;t++){
		v[t-t0] = variances[t];
	}

	pthread_mutex_unlock(&data_lock);

	return true;}

/**
 * returns variance vector for most recent measurements
 */
bool EmotivAffectivEEG::variance(std::vector<float>& v) const
{
	pthread_mutex_lock(&data_lock);

	v = latest_variance;

	pthread_mutex_unlock(&data_lock);

	return true;
}


/**
 * return number of 1d-signals in data
 */
unsigned int EmotivAffectivEEG::getNumberOfSignals() const
{
	return NUMBER_OF_SIGNALS;
}

/**
 * return signal name for the given signal number
 */
std::string EmotivAffectivEEG::getSignalName(int index) const
{
	if(index == 0)
		return "Excitement";
	else if(index == 1)
		return "Meditation";
	else if(index == 2)
		return "Frustration";
	else if(index == 3)
		return "Engagement-Boredom";
	else
		return "N/A";
}

/**
 * Returns amount of data collected [collection frequency is 1 Hz]
 */
int EmotivAffectivEEG::getDataLength() const
{
	int t = 0;

	pthread_mutex_lock(&data_lock);

	t = values.size();

	pthread_mutex_unlock(&data_lock);

	return t;
}


bool EmotivAffectivEEG::clearData()
{
	pthread_mutex_lock(&data_lock);

	values.clear();
	variances.clear();

	pthread_mutex_unlock(&data_lock);

	return true;
}


double EmotivAffectivEEG::current_time() const
{
	struct timeval tv;
	struct timezone tz;

	double t = 0.0;

	if(gettimeofday(&tv, &tz) == 0){
		t = ((double)tv.tv_sec);
		t += ((double)tv.tv_usec)/1000000.0;
	}

	// TODO: measure quarter of seconds or something and use it as integer time measure
	return time_frequency*t;
}


///////////////////////////////////////////////////////////////
// EEG data polling and processing

// generates data from observations and samples it to the time-line
void EmotivAffectivEEG::update_tick(double t0, double t1)
{
	if(t1 <= t0) return; // nothing to do
	unsigned int delta = (unsigned int)(t1 - t0);
	if(delta <= 0) return; // nothing to do

	//////////////////////////////////////////////////////////////
	// process data

	pthread_mutex_lock(&data_lock);

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

	pthread_mutex_unlock(&data_lock);

	//////////////////////////////////////////////////////////////

	// reset
	times.clear();
	samples.clear();
	buffer_min_time = +100000000.0f;
	buffer_max_time = -100000000.0f;
}

// handles new data entry
void EmotivAffectivEEG::new_data(float t, std::vector<float>& data)
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


void EmotivAffectivEEG::poll_events()
{
	int poll_freq = 20; // Hz

	double t0 = current_time();

	latest_data_received_t = current_time();

	while(keep_polling){

		// 1. check that there is connection
		while(connection != EDK_OK){
			pthread_mutex_lock(&emotiv_lock);
			connection = EE_EngineConnect();
			pthread_mutex_unlock(&emotiv_lock);

			microsleep(1000/poll_freq);

			double t1 = current_time();
			if(t1 - t0 >= 1.0){ update_tick(t0, t1); t0 = t1; }
		}

		pthread_mutex_lock(&emotiv_lock);

		// 2. once we have connection, poll for events
		// TODO: how to notice if the connection goes down??
		int state = EE_EngineGetNextEvent(eEvent);

		while(state == EDK_OK){

			EE_Event_t event_type = EE_EmoEngineEventGetType(eEvent);
			// EE_EmoEngineEventGetUserId(eEvent, &userID);

			if(event_type == EE_EmoStateUpdated){
				EE_EmoEngineEventGetEmoState(eEvent, eState);

				// get affectiv scores as they are most usable
				std::vector<float> scores;

				float t = ES_GetTimeFromStart(eState);

				if(ES_AffectivIsActive(eState, AFF_EXCITEMENT))
					scores.push_back(ES_AffectivGetExcitementShortTermScore(eState));
				else scores.push_back(0.5f);

				// ignore ES_AffectivGetExcitementLongTermScore()

				if(ES_AffectivIsActive(eState, AFF_MEDITATION))
					scores.push_back(ES_AffectivGetMeditationScore(eState));
				else scores.push_back(0.5f);

				if(ES_AffectivIsActive(eState, AFF_FRUSTRATION))
					scores.push_back(ES_AffectivGetFrustrationScore(eState));
				else scores.push_back(0.5f);

				if(ES_AffectivIsActive(eState, AFF_ENGAGEMENT_BOREDOM))
					scores.push_back(ES_AffectivGetEngagementBoredomScore(eState));
				else scores.push_back(0.5f);

				/////////////////////////////////////////////////////////////////
				// process collected data here [add to datastream we are maintaining]

				latest_data_received_t = current_time();

				new_data(t, scores);
			}

			state = EE_EngineGetNextEvent(eEvent);
		}

		pthread_mutex_unlock(&emotiv_lock);

		microsleep(1000/poll_freq);

		double t1 = current_time();
		if(t1 - t0 >= 1.0){ update_tick(t0, t1); t0 = t1; }
	}

}



void* __emotiv_start_poll_thread(void* ptr)
{
	if(ptr == NULL) return NULL;

	EmotivAffectivEEG* p = (EmotivAffectivEEG*)ptr;

	p->poll_events();

	return NULL;
}



