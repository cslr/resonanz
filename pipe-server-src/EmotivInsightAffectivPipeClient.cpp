/*
 * EmotivInsightAffectivPipeClient.cpp
 *
 *  Created on: 14.6.2015
 *      Author: Tomas
 */

#include "EmotivInsightAffectivPipeClient.h"
#include <iostream>

namespace whiteice {
namespace resonanz {

EmotivInsightAffectivPipeClient::EmotivInsightAffectivPipeClient(const std::string& pipename) {
	this->pipename = pipename;

	std::cout << "PIPE CLIENT PIPENAME = " << pipename << std::endl;

	hPipe = NULL;
}

EmotivInsightAffectivPipeClient::~EmotivInsightAffectivPipeClient() {
	if(hPipe != NULL && hPipe != INVALID_HANDLE_VALUE)
		CloseHandle(hPipe);
}

void EmotivInsightAffectivPipeClient::new_data(float t, std::vector<float>& data)
{
	// always tries to write to pipe and if it fails tries to close and reopen pipe
	{
		DWORD bytesWritten;

		// std::cout << "PIPE CLIENT: ABOUT TO CREATEFILE" << std::endl;

		hPipe = CreateFile(pipename.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if(hPipe == INVALID_HANDLE_VALUE){
			// std::cout << "PIPE CLIENT: INVALID HANDLE" << std::endl;
			return;
		}

		auto success = WriteFile(hPipe, data.data(), data.size()*sizeof(float), &bytesWritten, NULL);

		// std::cout << "PIPE CLIENT: DATA SUCCESSFULLY WRITTEN (2)" << std::endl;

		CloseHandle(hPipe);
	}
}

} /* namespace resonanz */
} /* namespace whiteice */
