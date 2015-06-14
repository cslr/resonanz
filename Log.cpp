/*
 * Log.cpp
 *
 *  Created on: 14.6.2015
 *      Author: Tomas
 */

#include "Log.h"
#include <stdexcept>
#include <exception>
#include <chrono>
#include <mutex>

#include <stdio.h>
#include <assert.h>


namespace whiteice {
namespace resonanz {

Log logging("renaissance-engine.log");


Log::Log(std::string logFilename)
{
	t0 = clock::now();

	handle = nullptr;
	handle = fopen(logFilename.c_str(), "wt");

	if(handle == 0) fprintf(stderr, "F: Starting logging mechanism failed: %s\n", logFilename.c_str());
	assert(handle != 0);

	buffer = (char*)malloc(BUFLEN*sizeof(char));

	info("Logging facilities started..");
}

Log::~Log() {
	info("Shutdown? Logging class destroyed.");

	fclose(handle);
	free(buffer);
	handle = nullptr;
	buffer = nullptr;
}


// logging
void Log::info(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "INFO %5.5f %s\n", ms, msg.c_str());
	fputs(buffer, handle);
}


void Log::warn(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "WARN %5.5f %s\n", ms, msg.c_str());
	fputs(buffer, handle);
}

void Log::error(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "ERRO %5.5f %s\n", ms, msg.c_str());
	fputs(buffer, handle);
}

void Log::fatal(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "FATA %5.5f %s\n", ms, msg.c_str());
	fputs(buffer, handle);
}


} /* namespace resonanz */
} /* namespace whiteice */
