/*
 * fi_iki_nop_neuromancer_ResonanzEngine.cpp
 *
 * JNI interface functions to send working commands to ResonanzEngine
 *
 *  Created on: 12.6.2015
 *      Author: Tomas
 */

#include "fi_iki_nop_neuromancer_ResonanzEngine.h"
#include "ResonanzEngine.h"
#include <jni.h>

// starts new engine (note: if thread creation fails we are in trouble)
static whiteice::resonanz::ResonanzEngine engine;

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startRandomStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startRandomStimulation
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile){

	if(env->IsSameObject(pictureDir, NULL)) return (jboolean)false;
	if(env->IsSameObject(keywordsFile, NULL)) return (jboolean)false;

	const char *pic = env->GetStringUTFChars(pictureDir, 0);
	const char *key = env->GetStringUTFChars(keywordsFile, 0);

	bool result = engine.cmdRandom(std::string(pic), std::string(key));

	env->ReleaseStringUTFChars(pictureDir, pic);
	env->ReleaseStringUTFChars(keywordsFile, key);

	return (jboolean)result;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startMeasureStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureStimulation
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile, jstring measurementsDir){

	if(env->IsSameObject(pictureDir, NULL)) return (jboolean)false;
	if(env->IsSameObject(keywordsFile, NULL)) return (jboolean)false;
	if(env->IsSameObject(measurementsDir, NULL)) return (jboolean)false;

	const char *pic = env->GetStringUTFChars(pictureDir, 0);
	const char *key = env->GetStringUTFChars(keywordsFile, 0);
	const char *mod = env->GetStringUTFChars(measurementsDir, 0);

	bool result = engine.cmdMeasure(std::string(pic), std::string(key), std::string(mod));

	env->ReleaseStringUTFChars(pictureDir, pic);
	env->ReleaseStringUTFChars(keywordsFile, key);
	env->ReleaseStringUTFChars(measurementsDir, mod);

	return (jboolean)result;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startOptimizeModel
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startOptimizeModel
  (JNIEnv * env, jobject obj, jstring modelDir)
{
	if(env->IsSameObject(modelDir, NULL)) return (jboolean)false;

	const char *mod = env->GetStringUTFChars(modelDir, 0);

	bool result = engine.cmdOptimizeModel(std::string(mod));

	env->ReleaseStringUTFChars(modelDir, mod);

	return (jboolean)result;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getOptimizeModelStatus
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getOptimizeModelStatus
  (JNIEnv * env, jobject obj)
{
	std::string line = engine.getEngineStatus();

	jstring result = env->NewStringUTF(line.c_str());

	return result;
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    stopOptimizeModel
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_stopOptimizeModel
  (JNIEnv * env, jobject obj)
{
	return (jboolean)engine.cmdStopOptimizeModel();
}



