/*
 * EmotivInsightAffectivPipeClient.h
 *
 *  Created on: 14.6.2015
 *      Author: Tomas
 */

#ifndef EMOTIVINSIGHTAFFECTIVPIPECLIENT_H_
#define EMOTIVINSIGHTAFFECTIVPIPECLIENT_H_

#include "EmotivInsightAffectiv.h"
#include <windows.h>

namespace whiteice {
namespace resonanz {

class EmotivInsightAffectivPipeClient: public EmotivInsightAffectiv {
public:
	EmotivInsightAffectivPipeClient(const std::string& pipename);
	virtual ~EmotivInsightAffectivPipeClient();

protected:
	void new_data(float t, std::vector<float>& data); // overrides new_data collection

	std::string pipename;

	HANDLE hPipe;
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* EMOTIVINSIGHTAFFECTIVPIPECLIENT_H_ */
