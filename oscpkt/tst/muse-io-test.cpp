/*
 * muse-io-test.cpp
 *
 *  Created on: 6.7.2015
 *      Author: Tomas
 */

#include <iostream>

#include "oscpkt.h"
#include "udp.h"

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

	std::cout << "MUSE TEST OSC (UDP PORT 4545) LISTENER" < std::endl;

	UdpSocket sock;
	sock.bindTo(port);

	if(!sock.isOk()) return; // failure [retry?]

	PacketReader pr;
	PacketWriter pw;

	while(sock.isOk()){
		if(sock.receiveNextPacket(30)){
			pr.init(sock.packetData(), sock.packetSize());
			Message* msg;
			while(pr.isOk() && ((msg = pr.popMessage()) != 0)){
				Message::ArgReader r = msg->match("/muse/elements/is-good");

				while(r.nbArgRemaining() > 0 && r.isOk()){

					if(r.isFloat()){
						float f;
						r = r.popFloat(f);
						std::cout << "f = " << f << std::endl;
					}
					else if(r.isDouble()){
						double f;
						r  = r.popDouble(f);
						std::cout << "d = " << f << std::endl;
					}
					else if(r.isInt32()){
						int32_t i;

						r = r.popInt32(i);
						std::cout << "i = " << i << std::endl;
					}
					else if(r.isInt64()){
						int64_t i;

						r = r.popInt64(i);
						std::cout << "i = " << i << std::endl;
					}
					else{
						std::cout << "unknown element" << std::endl;
					}
				}
			}
		}
	}

}

