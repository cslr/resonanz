/*
 * timing.h
 *
 *  Created on: 3.3.2013
 *      Author: omistaja
 */

#ifndef TIMING_H_
#define TIMING_H_

// works around buggy usleep in MINGW/windows [use microsleep instead]

#ifdef WIN32
#include <windows.h>
#define microsleep(msec) Sleep(msec)
#else
#include <unistd.h>
#define microsleep(msec) usleep(msec*1000);
#endif


#endif /* TIMING_H_ */
