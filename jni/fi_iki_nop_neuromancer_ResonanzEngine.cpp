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
#include "Log.h"
#include <jni.h>
#include <exception>


// starts new engine (note: if thread creation fails we are in trouble)
static whiteice::resonanz::ResonanzEngine engine;

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startRandomStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startRandomStimulation
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile)
{
	try{
		if(env->IsSameObject(pictureDir, NULL)) return (jboolean)false;
		if(env->IsSameObject(keywordsFile, NULL)) return (jboolean)false;

		const char *pic = env->GetStringUTFChars(pictureDir, 0);
		const char *key = env->GetStringUTFChars(keywordsFile, 0);

		bool result = engine.cmdRandom(std::string(pic), std::string(key));

		env->ReleaseStringUTFChars(pictureDir, pic);
		env->ReleaseStringUTFChars(keywordsFile, key);

		return (jboolean)result;
	}
	catch(std::exception& e){ return (jboolean)false; }
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startMeasureStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureStimulation
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile, jstring measurementsDir)
{
	try{
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
	catch(std::exception& e){ return (jboolean)false; }
}



JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startOptimizeModel
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile, jstring measurementsDir)
{
	try{
		if(env->IsSameObject(pictureDir, NULL)) return (jboolean)false;
		if(env->IsSameObject(keywordsFile, NULL)) return (jboolean)false;
		if(env->IsSameObject(measurementsDir, NULL)) return (jboolean)false;

		const char *pic = env->GetStringUTFChars(pictureDir, 0);
		const char *key = env->GetStringUTFChars(keywordsFile, 0);
		const char *mod = env->GetStringUTFChars(measurementsDir, 0);

		bool result = engine.cmdOptimizeModel(std::string(pic), std::string(key), std::string(mod));

		env->ReleaseStringUTFChars(pictureDir, pic);
		env->ReleaseStringUTFChars(keywordsFile, key);
		env->ReleaseStringUTFChars(measurementsDir, mod);

		return (jboolean)result;
	}
	catch(std::exception& e){ return (jboolean)false; }
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startExecuteProgram
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[[F)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startExecuteProgram
  (JNIEnv * env, jobject jobj,
		  jstring pictureDir, jstring keywordsFile, jstring modelDir, jstring audioFile,
		  jobjectArray targetNames, jobjectArray programs, jboolean blindMode, jboolean saveVideo)
{
	try{
		bool result = true;

		if(env->IsSameObject(pictureDir, NULL)) return (jboolean)false;
		if(env->IsSameObject(keywordsFile, NULL)) return (jboolean)false;
		if(env->IsSameObject(modelDir, NULL)) return (jboolean)false;
		if(env->IsSameObject(audioFile, NULL)) return (jboolean)false;

		if(env->GetArrayLength(targetNames) != env->GetArrayLength(programs))
			return (jboolean)false;

		const char *pic   = env->GetStringUTFChars(pictureDir, 0);
		const char *key   = env->GetStringUTFChars(keywordsFile, 0);
		const char *mod   = env->GetStringUTFChars(modelDir, 0);
		const char *audio = env->GetStringUTFChars(audioFile, 0);

		// load arrays
		const jsize length = env->GetArrayLength(targetNames);

		std::vector<std::string> targets;
		std::vector< std::vector<float> > progs;

		targets.resize((unsigned int)length);
		progs.resize((unsigned int)length);

		for(unsigned int i=0;i<(unsigned int)length;i++){
			jstring name = (jstring) env->GetObjectArrayElement(targetNames, i);
			const char* n = env->GetStringUTFChars(name, 0);
			targets[i] = n;
			env->ReleaseStringUTFChars(name, n);

			jfloatArray arr = (jfloatArray) env->GetObjectArrayElement(programs, i);
			progs[i].resize((unsigned int)env->GetArrayLength(arr));

			jfloat *body = env->GetFloatArrayElements(arr, 0);

			for(unsigned int j=0;j<progs[i].size();j++){
				progs[i][j] = (float)body[j];
			}

			env->ReleaseFloatArrayElements(arr, body, 0);
		}

		bool monteCarloMode = (bool)blindMode;
		bool saveVideoMode   = (bool)saveVideo;

		result = engine.cmdExecuteProgram(std::string(pic), std::string(key), std::string(mod),
				std::string(audio), targets, progs, monteCarloMode, saveVideoMode);

		env->ReleaseStringUTFChars(pictureDir, pic);
		env->ReleaseStringUTFChars(keywordsFile, key);
		env->ReleaseStringUTFChars(modelDir, mod);
		env->ReleaseStringUTFChars(audioFile, audio);

		return (jboolean)result;
	}
	catch(std::exception& e){ return (jboolean)false; }
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    isBusy
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_isBusy
  (JNIEnv * env, jobject jobj)
{
	return (jboolean)engine.isBusy();
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getStatusLine
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getStatusLine
  (JNIEnv * env, jobject obj)
{
	try{
		std::string line = engine.getEngineStatus();

		jstring result = env->NewStringUTF(line.c_str());

		return result;
	}
	catch(std::exception& e){ return (jboolean)false; }
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    stopCommand
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_stopCommand
  (JNIEnv * env, jobject obj)
{
	try{ return (jboolean)engine.cmdStopCommand(); }
	catch(std::exception& e){ return (jboolean)false; }
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getAnalyzeModel
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getAnalyzeModel
  (JNIEnv * env, jobject obj, jstring modelDir)
{
	try{
		if(env->IsSameObject(modelDir, NULL)) return (jboolean)false;

		const char *mod = env->GetStringUTFChars(modelDir, 0);

		std::string line = engine.analyzeModel(std::string(mod));

		env->ReleaseStringUTFChars(modelDir, mod);

		jstring result = env->NewStringUTF(line.c_str());

		return result;
	}
	catch(std::exception& e){
		std::string msg = "Internal software error: ";
		msg += e.what();

		jstring result = env->NewStringUTF(msg.c_str());
		return result;
	}
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getDeltaStatistics
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getDeltaStatistics
  (JNIEnv * env, jobject obj, jstring pictureDir, jstring keywordsFile, jstring modelDir)
{
	try{
		if(env->IsSameObject(pictureDir, NULL)) return (jboolean)false;
		if(env->IsSameObject(keywordsFile, NULL)) return (jboolean)false;
		if(env->IsSameObject(modelDir, NULL)) return (jboolean)false;

		const char *pic   = env->GetStringUTFChars(pictureDir, 0);
		const char *key   = env->GetStringUTFChars(keywordsFile, 0);
		const char *mod   = env->GetStringUTFChars(modelDir, 0);

		std::string line = engine.deltaStatistics(std::string(pic), std::string(key), std::string(mod));

		env->ReleaseStringUTFChars(pictureDir, pic);
		env->ReleaseStringUTFChars(keywordsFile, key);
		env->ReleaseStringUTFChars(modelDir, mod);

		jstring result = env->NewStringUTF(line.c_str());

		return result;
	}
	catch(std::exception& e){
		std::string msg = "Internal software error: ";
		msg += e.what();

		jstring result = env->NewStringUTF(msg.c_str());
		return result;
	}
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getLastExecutedProgramStatisics
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getLastExecutedProgramStatisics
  (JNIEnv * env, jobject obj)
{
	try{
		std::string line = engine.executedProgramStatistics();

		jstring result = env->NewStringUTF(line.c_str());

		return result;
	}
	catch(std::exception& e){
		std::string msg = "Internal software error: ";
		msg += e.what();

		jstring result = env->NewStringUTF(msg.c_str());
		return result;
	}
}



/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    deleteModelData
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_deleteModelData
 (JNIEnv * env, jobject obj, jstring modelDir)
{
	try{
		if(env->IsSameObject(modelDir, NULL)) return (jboolean)false;

		const char *mod = env->GetStringUTFChars(modelDir, 0);

		bool result = engine.deleteModelData(std::string(mod));

		env->ReleaseStringUTFChars(modelDir, mod);

		return (jboolean)result;

	}
	catch(std::exception& e){ return (jboolean)false; }
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    setEEGSourceDevice
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_setEEGSourceDevice
  (JNIEnv * env, jobject obj, jint deviceNumber)
{
	try{
		return (jboolean)engine.setEEGDeviceType((int)deviceNumber);
	}
	catch(std::exception& e){ return (jboolean)false; }

}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getEEGSourceDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getEEGSourceDevice
  (JNIEnv * env, jobject obj)
{
	try{
		return (jint)engine.getEEGDeviceType();
	}
	catch(std::exception& e){ return (jint)(-1); }
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getEEGDeviceStatus
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getEEGDeviceStatus
  (JNIEnv * env, jobject obj)
{
	try{
		std::string status;

		engine.getEEGDeviceStatus(status);

		jstring result = env->NewStringUTF(status.c_str());

		return result;
	}
	catch(std::exception& e){
		std::string error = "Internal error: ";
		error += e.what();

		jstring result = env->NewStringUTF(error.c_str());
		return result;
	}
}

