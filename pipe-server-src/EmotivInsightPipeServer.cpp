/*
 * EmotivInsightPipeServer.cpp
 *
 *  Created on: 1.7.2015
 *      Author: Tomas
 */

#include "EmotivInsightPipeServer.h"
#include "timing.h"
#include <windows.h>

#include <iostream>


namespace whiteice {
namespace resonanz {

EmotivInsightPipeServer::EmotivInsightPipeServer(const std::string& pipename)
{
	this->pipename = pipename;

}

EmotivInsightPipeServer::~EmotivInsightPipeServer() {
	keep_polling = false;
}


// poll events from clients sending data through pipe
void EmotivInsightPipeServer::poll_events()
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;

	int poll_freq = 20; // Hz
	double t0 = current_time();

	while(keep_polling){

		while(hPipe == INVALID_HANDLE_VALUE && keep_polling){
			hPipe = CreateNamedPipe(
					pipename.c_str(),
					PIPE_ACCESS_INBOUND,
					// non-blocking pipe reads (deprecated but we use it anyway..)
					PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
					10,
					1024, // buffer sizes
					1024,
					0, // default time out: 50ms
					NULL);

			microsleep(1000/poll_freq);

			double t1 = current_time();
			if(t1 - t0 >= 1.0){ update_tick(t0, t1); t0 = t1; }
		}

		// printf("SERVER LOOP: %d\n", (int)hPipe);
		// fflush(stdout);


		if(ConnectNamedPipe(hPipe, NULL)){
			float buffer[256];
			DWORD dwRead = 0;

			//printf("CLIENT CONNECTION: READFILE");
			//fflush(stdout);

			while(ReadFile(hPipe, buffer, sizeof(buffer), &dwRead, NULL) != FALSE){
				dwRead /= sizeof(float);

				//printf("READFILE READ %d FLOATS\n", (int)dwRead);

				if(dwRead == NUMBER_OF_SIGNALS){ // should receive 5 floats per message
					std::vector<float> data;

					//printf("EMOTIV INSIGHT SERVER RECEIVED DATA: ");
					for(unsigned int i=0;i<dwRead;i++){
						//printf("%f ", buffer[i]);
						data.push_back(buffer[i]);
					}
					//printf("\n");

					// received new emotiv insight signal values so sends them for processing
					latest_data_received_t = current_time();

					float t = current_time();

					new_data(t, data);
				}
			}

			DisconnectNamedPipe(hPipe); // disconnects the previous connection
		}

		fflush(stdout);

		double t1 = current_time();
		if(t1 - t0 >= 1.0){ update_tick(t0, t1); t0 = t1; }

		microsleep(1000/poll_freq);
	}

}



} /* namespace resonanz */
} /* namespace whiteice */
