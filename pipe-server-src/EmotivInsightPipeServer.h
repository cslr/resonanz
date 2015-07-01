/*
 * EmotivInsightPipeServer.h
 *
 *  Created on: 1.7.2015
 *      Author: Tomas
 */

#ifndef EMOTIVINSIGHTPIPESERVER_H_
#define EMOTIVINSIGHTPIPESERVER_H_

#include "EmotivInsightAffectiv.h"
#include <string>

namespace whiteice {
namespace resonanz {

class EmotivInsightPipeServer: public EmotivInsightAffectiv {
public:
	EmotivInsightPipeServer(const std::string& pipename);
	virtual ~EmotivInsightPipeServer();

protected:
	std::string pipename;

	void poll_events(); // poll events from clients sending data through pipe
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* EMOTIVINSIGHTPIPESERVER_H_ */
