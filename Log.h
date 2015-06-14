/*
 * Log.h
 *
 *  Created on: 14.6.2015
 *      Author: Tomas
 */

#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <string>
#include <stdio.h>

#include <chrono>
#include <mutex>

namespace whiteice {
namespace resonanz {

class Log {
public:
	Log(std::string logFilename);
	virtual ~Log();

	// logging
	void info(std::string msg);
	void warn(std::string msg);
	void error(std::string msg);
	void fatal(std::string msg);

protected:
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::milliseconds milliseconds;

	FILE* handle = nullptr;
	char* buffer = nullptr;
	static const int BUFLEN = 8192;

	clock::time_point t0;
	std::mutex file_lock;
};

extern Log logging;

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* LOG_H_ */
