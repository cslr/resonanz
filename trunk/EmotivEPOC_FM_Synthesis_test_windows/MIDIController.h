/*
 * MIDIController.h
 *
 * Class to send MIDI controller events from data source. Uses PortMIDI library for MIDI commands.
 *
 *  Created on: 1.3.2013
 *      Author: omistaja
 */

#ifndef MIDICONTROLLER_H_
#define MIDICONTROLLER_H_

#include <pthread.h>
#include "DataSource.h"
#include "portmidi.h"

class MIDIController {
public:
	MIDIController(const DataSource& source,
			unsigned int device,
			unsigned int channel = 0);
	virtual ~MIDIController();

	// returns true if data is being send to MIDI device from the source successfully
	bool sending() const;

	// set mute values for controllers [if muted no data will be sent]
	bool setMute(const std::vector<bool>& controllers);
	bool getMute(std::vector<bool>& controllers);

	bool setMute(unsigned int index, bool mute);

	// set mute for all controllers
	void setMute(bool mute);

	unsigned int getNumberOfControllers();

private:
	const DataSource& source;

	unsigned int midi_device;
	unsigned int channel; // midi channel to use
	PmStream* midi;
	PmEvent event[8];

	void poll_loop();

	bool sending_data;
	bool keep_polling;
	pthread_t poll_thread;
	pthread_mutex_t portmidi_lock;
	std::vector<bool> mute;

	friend void* __midicontroller_thread(void* ptr);
};

#endif /* MIDICONTROLLER_H_ */
