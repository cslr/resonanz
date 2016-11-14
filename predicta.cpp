// user interface for JNI native library

#include "fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative.h"
#include <stdio.h>


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif



/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    startOptimization
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;D)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_startOptimization
  (JNIEnv *env, jobject obj, jstring trainingFile, jstring scoringFile, jstring resultsFile, jdouble riskLevel)
{
  return (jboolean)FALSE;
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    getRunning
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_getRunning
  (JNIEnv *env, jobject obj)
{
  return (jboolean)FALSE;
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    stopOptimization
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_stopOptimization
  (JNIEnv * env, jobject obj)
{
  return (jboolean)FALSE;
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    getError
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_getError
  (JNIEnv * env, jobject obj)
{
  return env->NewStringUTF("<not implemented>");
}

/*
 * Class:     fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative
 * Method:    getStatus
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_novelinsight_predicta_PredictaOptimizerNative_getStatus
  (JNIEnv * env, jobject obj)
{
  return env->NewStringUTF("<predicta.dll loaded>");
}

