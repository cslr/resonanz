/*
 * muse-io-test.cpp
 *
 *  Created on: 6.7.2015
 *      Author: Tomas
 */

#include <iostream>
#include <vector>
#include <math.h>

#include "oscpkt.hh"
#include "udp.hh"

using namespace oscpkt;

/**
 * Fetches muse-io udp packets from port localhost:4545 and reads OSC paths:
 * /muse/elements/delta_relative
 * /muse/elements/theta_relative
 * /muse/elements/alpha_relative
 * /muse/elements/beta_relative
 * /muse/elements/gamma_relative
 *
 * Calculates mean delta, theta, alpha, beta and gamma values (and st.dev.)
 * from all channels and which /muse/elements/is_good value is >= 0.5.
 * If there are no good values at all, reports 0.5 value and maximum st.dev.
 * (of uniform distribution between [0,1])
 *
 * Also measures:
 * /muse/elements/experimental/concentration
 * /muse/elements/experimental/mellow
 */
int main(int argc, char** argv)
{
	const unsigned int port = 4545;

	std::cout << "MUSE TEST OSC (UDP PORT 4545) LISTENER" << std::endl;
	fflush(stdout);

	UdpSocket sock;
	sock.bindTo(port);

	if(!sock.isOk()) return -1; // failure [retry?]

	PacketReader pr;
	PacketWriter pw;

	std::vector<int> connectionQuality;
	bool hasConnection = false;
	float delta = 0.0f, theta = 0.0f, alpha = 0.0f, beta = 0.0f, gamma = 0.0f;

	while(sock.isOk()){

		if(hasConnection){
			float q = 0.0f;
			for(auto qi : connectionQuality)
				if(qi > 0) q++;
			q = q / connectionQuality.size();

			// printf("EEQ: D:%.2f T:%.2f A:%.2f B:%.2f G:%.2f [QUALITY: %.2f]\n", delta, theta, alpha, beta, gamma, q);
			printf("EEQ POWER: %.2f [QUALITY %.2f]\n",
					log(exp(delta)+exp(theta)+exp(alpha)+exp(beta)+exp(gamma)), q);

			fflush(stdout);
		}

		if(sock.receiveNextPacket(30)){
			pr.init(sock.packetData(), sock.packetSize());
			Message* msg;
			while(pr.isOk() && ((msg = pr.popMessage()) != 0)){
				Message::ArgReader r = msg->match("/muse/elements/is_good");

				if(r.isOk() == true){ // matched
					// there are 4 ints telling connection quality
					std::vector<int> quality;

					while(r.nbArgRemaining()){
						if(r.isInt32()){
							int32_t i;
							r = r.popInt32(i);
							quality.push_back((int)i);
						}
						else{
							r = r.pop();
						}
					}

					if(quality.size() > 0){
						connectionQuality = quality;
					}

					bool connection = false;

					for(auto q : quality)
						if(q > 0) connection = true;

					hasConnection = connection;
				}

				// gets relative frequency bands..

				r = msg->match("/muse/elements/delta_absolute");
				if(r.isOk()){
					float f1, f2, f3, f4;
					if(r.popFloat(f1).popFloat(f2).popFloat(f3).popFloat(f4).isOkNoMoreArgs()){
						std::vector<float> v;
						v.push_back(f1); v.push_back(f2); v.push_back(f3); v.push_back(f4);

						// calculates mean according to connection quality
						float mean = 0.0f;
						float samples = 0.0f;

						for(unsigned int i=0;i<connectionQuality.size() && i<v.size();i++){
							if(connectionQuality[i] > 0){
								mean += exp(v[i]);
								samples++;
							}
						}

						if(samples > 0.0f){
							mean /= samples;
							mean = log(mean);
						}
						else
							mean = 0.5f; // mean value without data

						delta = mean;
					}
				}

				r = msg->match("/muse/elements/theta_absolute");
				if(r.isOk()){
					float f1, f2, f3, f4;
					if(r.popFloat(f1).popFloat(f2).popFloat(f3).popFloat(f4).isOkNoMoreArgs()){
						std::vector<float> v;
						v.push_back(f1); v.push_back(f2); v.push_back(f3); v.push_back(f4);

						// calculates mean according to connection quality
						float mean = 0.0f;
						float samples = 0.0f;

						for(unsigned int i=0;i<connectionQuality.size() && i<v.size();i++){
							if(connectionQuality[i] > 0){
								mean += exp(v[i]);
								samples++;
							}
						}

						if(samples > 0.0f){
							mean /= samples;
							mean = log(mean);
						}
						else
							mean = 0.5f; // mean value without data

						theta = mean;
					}
				}

				r = msg->match("/muse/elements/alpha_absolute");
				if(r.isOk()){
					float f1, f2, f3, f4;
					if(r.popFloat(f1).popFloat(f2).popFloat(f3).popFloat(f4).isOkNoMoreArgs()){
						std::vector<float> v;
						v.push_back(f1); v.push_back(f2); v.push_back(f3); v.push_back(f4);

						// calculates mean according to connection quality
						float mean = 0.0f;
						float samples = 0.0f;

						for(unsigned int i=0;i<connectionQuality.size() && i<v.size();i++){
							if(connectionQuality[i] > 0){
								mean += exp(v[i]);
								samples++;
							}
						}

						if(samples > 0.0f){
							mean /= samples;
							mean = log(mean);
						}
						else
							mean = 0.5f; // mean value without data

						alpha = mean;
					}
				}


				r = msg->match("/muse/elements/beta_absolute");
				if(r.isOk()){
					float f1, f2, f3, f4;
					if(r.popFloat(f1).popFloat(f2).popFloat(f3).popFloat(f4).isOkNoMoreArgs()){
						std::vector<float> v;
						v.push_back(f1); v.push_back(f2); v.push_back(f3); v.push_back(f4);

						// calculates mean according to connection quality
						float mean = 0.0f;
						float samples = 0.0f;

						for(unsigned int i=0;i<connectionQuality.size() && i<v.size();i++){
							if(connectionQuality[i] > 0){
								mean += exp(v[i]);
								samples++;
							}
						}

						if(samples > 0.0f){
							mean /= samples;
							mean = log(mean);
						}
						else
							mean = 0.5f; // mean value without data

						beta = mean;
					}
				}

				r = msg->match("/muse/elements/gamma_absolute");
				if(r.isOk()){
					float f1, f2, f3, f4;
					if(r.popFloat(f1).popFloat(f2).popFloat(f3).popFloat(f4).isOkNoMoreArgs()){
						std::vector<float> v;
						v.push_back(f1); v.push_back(f2); v.push_back(f3); v.push_back(f4);

						// calculates mean according to connection quality
						float mean = 0.0f;
						float samples = 0.0f;

						for(unsigned int i=0;i<connectionQuality.size() && i<v.size();i++){
							if(connectionQuality[i] > 0){
								mean += exp(v[i]);
								samples++;
							}
						}

						if(samples > 0.0f){
							mean /= samples;
							mean = log(mean);
						}
						else
							mean = 0.5f; // mean value without data

						gamma = mean;
					}
				}

			}
		}
	}

	return 0;
}

