/*
 * MIDIController.cpp
 *
 *  Created on: 1.3.2013
 *      Author: omistaja
 */

#include "MIDIController.h"
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <stdexcept>
#include "timing.h"

#include "portmidi.h"


void* __midicontroller_thread(void* ptr);


MIDIController::MIDIController(const DataSource& __source,
		unsigned int device, unsigned int channel) : source(__source)
{
	keep_polling  = true;

	const PmDeviceInfo* info = Pm_GetDeviceInfo(device);
	assert(info != 0);
	assert(info->output != 0);
	assert(channel < 16);

	this->midi_device  = device;
	this->channel = channel;
	this->midi = NULL;
	this->sending_data = false;

	mute.resize(8);
	for(unsigned int i=0;i<8;i++){
		mute[i] = true; // don't send anything initially
	}

	if(pthread_mutex_init(&portmidi_lock, NULL) != 0){
		throw std::runtime_error("Cannot initialize emotiv mutex.");
	}

	// no latency [TODO: check for errors]
	if(Pm_OpenOutput(&midi, midi_device, NULL, 0, NULL, NULL, 0) != pmNoError){
		pthread_mutex_destroy(&portmidi_lock);
		throw std::runtime_error("Opening MIDI device failed (portmidi).");
	}


	if(pthread_create(&poll_thread, NULL, __midicontroller_thread, (void*)this) != 0){
		pthread_mutex_destroy(&portmidi_lock);
		Pm_Close(midi);
		throw std::runtime_error("Starting thread failed.");
	}
}

MIDIController::~MIDIController()
{
	sending_data = false;
	keep_polling = false;

	pthread_mutex_lock(&portmidi_lock);

	pthread_kill(poll_thread, SIGTERM);

	Pm_Close(midi);
	// Pm_Terminate();

	pthread_mutex_unlock(&portmidi_lock);

	pthread_mutex_destroy(&portmidi_lock);
}


bool MIDIController::sending() const
{
	return this->sending_data;
}


// set mute values for controllers [if muted no data will be sent]
bool MIDIController::setMute(const std::vector<bool>& controllers)
{
	unsigned int N = mute.size();
	if(controllers.size() < N)
		N = controllers.size();

	for(unsigned int i=0;i<N;i++)
		mute[i] = controllers[i];

	return true;
}


bool MIDIController::getMute(std::vector<bool>& controllers)
{
	controllers.resize(mute.size());

	for(unsigned int i=0;i<mute.size();i++)
		controllers[i] = mute[i];

	return true;
}


bool MIDIController::setMute(unsigned int index, bool mute_value)
{
	if(index < this->mute.size()){
		mute[index] = mute_value;
		return true;
	}
	else{
		return false;
	}
}


// set mute for all controllers
void MIDIController::setMute(bool mute)
{
	for(unsigned int i=0;i<this->mute.size();i++)
		this->mute[i] = mute;
}


unsigned int MIDIController::getNumberOfControllers()
{
	unsigned int d = source.getNumberOfSignals();

	if(d > 8)
		d = 8;

	return d;
}


void MIDIController::poll_loop()
{
	int update_freq = 5; // 5 Hz (200ms)

	std::vector<float> x;

	while(keep_polling){
		microsleep(1000/update_freq);

		if(source.data(x) == false) continue;

		unsigned int d = 8;
		if(x.size() < d) d = x.size();
		unsigned int m = 0;

		for(unsigned int i=0;i<d;i++){
			if(mute[i]) continue; // don't send this controller's value

			int v = (int)(127*x[i]);
			if(v > 127) v = 127;
			else if(v < 0) v = 0;

			if(i < 4){
				// controller values [0x10-0x13]: 0-127
				unsigned int cc = (0x10 + i);
				event[m].message = Pm_Message((0xB0+channel), cc, ((unsigned int)v));
				event[m].timestamp = 0;
				m++;
			}
			else{
				// controller values [0x50-0x53]: 0-127
				unsigned int cc = (0x50+(i-4));
				event[m].message = Pm_Message((0xB0+channel), cc, ((unsigned int)v));
				event[m].timestamp = 0;
				m++;
			}
		}


		if(m > 0){
			pthread_mutex_lock(&portmidi_lock);

			if(Pm_Write(midi, event, m) == pmNoError){
				sending_data = true;
			}
			else{
				sending_data = false;
			}

			pthread_mutex_unlock(&portmidi_lock);
		}
		else{
			sending_data = false;
		}

	}
}


void* __midicontroller_thread(void* ptr)
{
	if(ptr == NULL) return NULL;

	MIDIController* p = (MIDIController*)ptr;

	p->poll_loop();

	return NULL;
}
