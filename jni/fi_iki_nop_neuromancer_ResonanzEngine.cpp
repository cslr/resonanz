/*
 * fi_iki_nop_neuromancer_ResonanzEngine.cpp
 *
 * JNI interface functions to send working commands to ResonanzEngine
 *
 *  Created on: 12.6.2015
 *      Author: Tomas
 */

#include "fi_iki_nop_neuromancer_ResonanzEngine.h"

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startRandomStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startRandomStimulation
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile){

	// FIXME implement me

	return (jboolean)false;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startMeasureStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureStimulation
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile, jstring measurementsDir){

	// FIXME implement me

	return (jboolean)false;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startOptimizeModel
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startOptimizeModel
  (JNIEnv * env, jobject obj, jstring modelDir){

	// FIXME implement me

	return (jboolean)false;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getOptimizeModelStatus
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getOptimizeModelStatus
  (JNIEnv * env, jobject obj)
{
	// FIXME implement me

	return (jboolean)false;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    stopOptimizeModel
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_stopOptimizeModel
  (JNIEnv *, jobject obj){

	// FIXME implement me

	return (jboolean)false;
}



