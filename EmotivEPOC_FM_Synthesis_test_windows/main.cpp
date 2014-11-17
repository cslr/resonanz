/*
 * main.cpp
 *
 *  Created on: 6.2.2013
 *      Author: omistaja
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <stdint.h>

#include <windows.h>
#include "EmotivAffectivEEG.h"
#include "FMSoundSynthesis.h"
#include <time.h>
#include "SDL.h"
#include <math.h>

#include "MIDIController.h"
#include "SinusoidGenerator.h"
#include "timing.h"
#include "DataSource.h"


int WinMain(HINSTANCE hinst1, HINSTANCE hinst2,LPSTR str,int num){
	int argc = 0; char** argv = NULL;
	srand((int)time(0));
	return main(argc, argv);
}


int main(int argc, char** argv)
{
	printf("Neuromancer 0.13 <nop@iki.fi> [%d: %f %d %d]\n",
		(int)time(0), rand() / (double)RAND_MAX, rand(), RAND_MAX);

	/*
	for(int i=0;i<Pm_CountDevices();i++){
		const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
		if(info->output){
			printf("%d: %s (%s)\n", i, info->name, info->interf);
		}
	}

	int midi_id = 4;

	printf("Activating device %d.\n", midi_id);

	fflush(stdout);

	SinusoidGenerator gen;
	EmotivAffectivEEG* eeg = new EmotivAffectivEEG();
	DataSource* ds = eeg;

	MIDIController* midi = new MIDIController(*ds, midi_id);
	// MIDIController* midi = new MIDIController(gen, midi_id);

	midi->setMute(0, false);
	midi->setMute(1, false);

	time_t t0 = time(0);
	time_t t1 = t0;
	time_t t2 = t0;

	while(1){
		microsleep(500);
		t1 = time(0);

		if(t1 - t2 > 1){
			std::vector<float> x;

			if(ds->data(x)){
				printf("data vector = ");
				for(unsigned int i=0;i<x.size();i++){
				printf("%f ", x[i]);
				}
				printf("\n");
			}
			else printf("no data.");

			t2 = time(0);
		}

		if(t1 - t0 > 30) // 30 seconds
			break;
	}

	delete midi;
	delete eeg;
	*/


	EmotivAffectivEEG* eeg = new EmotivAffectivEEG();
	SDLSoundSynthesis* fm  = new FMSoundSynthesis();
	// SDLSoundSynthesis* fm = new AdditiveSoundSynthesis();

	std::vector<float> parameters;
	parameters.resize(fm->getNumberOfParameters());

	FILE* handle = fopen("session.log", "a+");
	if(handle == 0){
		delete eeg;
		delete fm;
		printf("ERROR: Cannot open session log file.\n");
		return -1;
	}

	printf("Press spacebar to exit..\n");

	while(GetAsyncKeyState(VK_SPACE) == 0) // waits for spacebar
	{
		parameters[0] = 0.5; // (rand() / (double)RAND_MAX);
		parameters[1] = 1000.0*(rand() / (double)RAND_MAX);
		parameters[2] = 1000.0*(rand() / (double)RAND_MAX);
		parameters[3] = 1000.0*(rand() / (double)RAND_MAX);

		fm->setParameters(parameters);
		fm->reset();
		eeg->clearData();

		fm->play();
		microsleep(20000); // listen sound for 20 seconds

		{
			std::vector< std::vector<float> > p;
			std::vector< float > r;

			eeg->data(p);

			if(p.size() > 0){
				r.resize(p[0].size());
				for(unsigned int j=0;j<r.size();j++)
					r[j] = 0.0f;

				for(unsigned int i=0;i<p.size();i++)
					for(unsigned int j=0;j<r.size();j++)
						r[j] += p[i][j];

				for(unsigned int j=0;j<r.size();j++)
					r[j] /= p.size();

				std::string line = "";
				char buffer[80];

				for(unsigned int j=0;j<parameters.size();j++){
					sprintf(buffer, "%f ", parameters[j]);
					line += buffer;
				}

				for(unsigned int j=0;j<r.size();j++){
					sprintf(buffer, "%f ", r[j]);
					line += buffer;
				}

				line += "\n";

				std::cout << line;
				fprintf(handle, line.c_str());
				fflush(stdout);
			}
		}

		fm->pause();
	}

	fm->pause();

	delete fm;
	delete eeg;
	fclose(handle);

	/*
	EmotivAffectivEEG* eeg = new EmotivAffectivEEG();

	if(eeg->connectionOk()){
		printf("connection to epoc device ok.\n");
	}
	else{
		printf("connection is down. shutdown.\n");
		return -1;
	}

	time_t t0 = time(0);
	time_t t1 = t0;

	while(1){
		usleep(500);
		t1 = time(0);

		if(t1 - t0 > 60) // 60 seconds
			break;
	}

	delete eeg;

	printf("Uptime %d seconds\n", (int)(t1 - t0));
	*/

	return 0;
}


int old_main(int argc, char** argv)
{
	std::cout << "Neuromancer v0.1" << std::endl;

	if(EE_EngineConnect() != EDK_OK){
		std::cout << "EmoEngine connect failed." << std::endl;
		return -1;
	}

	EmoEngineEventHandle eEvent = EE_EmoEngineEventCreate();
	EmoStateHandle eState       = EE_EmoStateCreate();
	unsigned int userID         = 0;

	const int poll_freq = 10; // Hz

	while(1){
		int state = EDK_OK;

		while(state == EDK_OK){
			state = EE_EngineGetNextEvent(eEvent);

			// if there is no connection there are not even events [data]

			if(state == EDK_OK){
				EE_Event_t event_type = EE_EmoEngineEventGetType(eEvent);
				EE_EmoEngineEventGetUserId(eEvent, &userID);

				if(event_type == EE_EmoStateUpdated){
					EE_EmoEngineEventGetEmoState(eEvent, eState);

					// get affectiv scores as they are most usable
					std::vector<float> scores;

					float t = ES_GetTimeFromStart(eState);

					if(ES_AffectivIsActive(eState, AFF_EXCITEMENT))
						scores.push_back(ES_AffectivGetExcitementShortTermScore(eState));
					else scores.push_back(-1.0f);

					// ignore ES_AffectivGetExcitementLongTermScore()

					if(ES_AffectivIsActive(eState, AFF_MEDITATION))
						scores.push_back(ES_AffectivGetMeditationScore(eState));
					else scores.push_back(-1.0f);

					if(ES_AffectivIsActive(eState, AFF_FRUSTRATION))
						scores.push_back(ES_AffectivGetFrustrationScore(eState));
					else scores.push_back(-1.0f);

					if(ES_AffectivIsActive(eState, AFF_ENGAGEMENT_BOREDOM))
						scores.push_back(ES_AffectivGetEngagementBoredomScore(eState));
					else scores.push_back(-1.0f);

					std::cout << t << " : ";
					for(std::vector<float>::iterator i = scores.begin();i != scores.end();i++){
						std::cout << *i << " ";
					}
					std::cout << std::endl;

				}
			}
		}

		Sleep(1000/poll_freq);
	}

	EE_EngineDisconnect();
	EE_EmoStateFree(eState);
	EE_EmoEngineEventFree(eEvent);

	return 0;
}


