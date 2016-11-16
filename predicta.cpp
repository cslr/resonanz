// user interface for JNI native library

#include <stdio.h>
#include "fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative.h"

#include "PredictaEngine.h"

#include <iostream>
#include <string>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//////////////////////////////////////////////////////////////////////
// starts prediction engine that calls use

static whiteice::resonanz::PredictaEngine engine;

//////////////////////////////////////////////////////////////////////



/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    startOptimization
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;D)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_startOptimization
(JNIEnv *env, jobject obj, jstring trainingFile_, jstring scoringFile_, jstring resultsFile_, jdouble riskLevel_,
 jdouble optimizationTime_, jboolean demoVersion_)
{
  const char *str1 = env->GetStringUTFChars(trainingFile_, 0);
  const char *str2 = env->GetStringUTFChars(scoringFile_, 0);
  const char *str3 = env->GetStringUTFChars(resultsFile_, 0);

  std::string trainingFile = str1;
  std::string scoringFile  = str2;
  std::string resultsFile  = str3;
  double risk = riskLevel_;
  double optimizationTime = optimizationTime_;
  bool demo = (bool)demoVersion_;

  env->ReleaseStringUTFChars(trainingFile_, str1);
  env->ReleaseStringUTFChars(scoringFile_ , str2);
  env->ReleaseStringUTFChars(resultsFile_ , str3);

  bool rv = engine.startOptimization(trainingFile, scoringFile, resultsFile, risk, optimizationTime, demo);

  return (jboolean)rv;
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    getRunning
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_getRunning
  (JNIEnv *env, jobject obj)
{
  // printf("getRunning() called\n"); fflush(stdout);

  bool rv = engine.isActive();
  
  return (jboolean)rv;
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    stopOptimization
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_stopOptimization
  (JNIEnv * env, jobject obj)
{
  // printf("stopOptimization()\n"); fflush(stdout);
  
  bool rv = engine.stopOptimization();
  
  return (jboolean)rv;
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    getError
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_getError
  (JNIEnv * env, jobject obj)
{
  std::string rv = engine.getError();
  
  return env->NewStringUTF(rv.c_str());
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    getStatus
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_getStatus
  (JNIEnv * env, jobject obj)
{
  std::string rv = engine.getStatus();
  
  return env->NewStringUTF(rv.c_str());
}

