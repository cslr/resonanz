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

Log logging("resonanz-engine.log");


Log::Log(std::string logFilename)
{
	{
	  std::lock_guard<std::mutex> lock(file_lock);
	  
	  t0 = clock::now();

	  handle = nullptr;
	  handle = fopen(logFilename.c_str(), "wt");
	  
	  if(handle == 0) fprintf(stderr, "F: Starting logging mechanism failed: %s\n", logFilename.c_str());
	  // assert(handle != 0);
	}

	info("Logging facilities started..");
}

Log::~Log() {
	info("Shutdown? Logging class destroyed.");

	fclose(handle);
	handle = nullptr;
}


// logging
void Log::info(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "INFO %5.5f %s\n", ms, msg.c_str());
	if(handle){ fputs(buffer, handle); fflush(handle); }	  
}


void Log::warn(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "WARN %5.5f %s\n", ms, msg.c_str());
	if(handle){ fputs(buffer, handle); fflush(handle); }	  
}

void Log::error(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "ERRO %5.5f %s\n", ms, msg.c_str());
	if(handle){ fputs(buffer, handle); fflush(handle); }
}

void Log::fatal(std::string msg){
	std::lock_guard<std::mutex> lock(file_lock);
	double ms = std::chrono::duration_cast<milliseconds>(clock::now() - t0).count()/1000.0;
	snprintf(buffer, BUFLEN, "FATA %5.5f %s\n", ms, msg.c_str());
	if(handle){ fputs(buffer, handle); fflush(handle); }
}


} /* namespace resonanz */
} /* namespace whiteice */
