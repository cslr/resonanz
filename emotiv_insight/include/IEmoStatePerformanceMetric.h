/*
 * Hand hacked header files that should be in .lib files..
 *
 */

#ifndef IEDK_PERF_H
#define IEDK_PERF_H

#include "Iedk.h"

#ifdef __cplusplus
extern "C" {
#endif

  
  EDK_API void IS_PerformanceMetricGetStressModelParams(EmoStateHandle eState,
							double* raw,
							double* min,
							double* max);
  
  EDK_API void IS_PerformanceMetricGetEngagementBoredomModelParams(EmoStateHandle eState,
							double* raw,
							double* min,
							double* max);

  EDK_API void IS_PerformanceMetricGetRelaxationModelParams(EmoStateHandle eState,
							    double* raw,
							    double* min,
							    double* max);
  
  EDK_API void IS_PerformanceMetricGetInstantaneousExcitementModelParams(EmoStateHandle eState,
									 double* raw,
									 double* min,
									 double* max);
  
  EDK_API void IS_PerformanceMetricGetInterestModelParams(EmoStateHandle eState,
							  double* raw,
							  double* min,
							  double* max);
  
  
  
#ifdef __cplusplus
}
#endif
#endif // IEDK_H
