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

#include "Log.h"


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
 * Method:    startMeasureProgram
 * Signature: (Ljava/lang/String;[Ljava/lang/String;I)[[F
 */
// JNIEXPORT jobjectArray JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureProgram
//  (JNIEnv * env, jobject obj, jstring mediaFile, jobjectArray signalNames, jint programLengthTicks)
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureProgram
  (JNIEnv *env, jobject obj, jstring mediaFile, jobjectArray signalNames, jint programLengthTicks)
{
	try{
		std::vector< std::vector<float> > program; // program that will be measured
		std::vector<std::string> names; // signal names that will be collected
		std::string media; // media filenames

		if(env->IsSameObject(mediaFile, NULL)) return (jboolean)false;
		if(env->IsSameObject(signalNames, NULL)) return (jboolean)false;

		const char *m     = env->GetStringUTFChars(mediaFile, 0);
		media = m;

		// load arrays
		const jsize length = env->GetArrayLength(signalNames);

		names.resize((unsigned int)length);
		program.resize((unsigned int)length);

		for(unsigned int i=0;i<(unsigned int)length;i++){
			jstring name = (jstring) env->GetObjectArrayElement(signalNames, i);
			const char* n = env->GetStringUTFChars(name, 0);
			names[i] = n;
			env->ReleaseStringUTFChars(name, n);
		}

		for(unsigned int i=0;i<program.size();i++){
			program[i].resize(programLengthTicks);
			for(auto& v : program[i])
				v = -1.0f; // intepolation/no data points initially
		}

		////////////////////////////////////////////////////////////////////////////
		// measure real program values here

		if(engine.cmdMeasureProgram(media, names, programLengthTicks) == false)
			return (jboolean)false; // something went wrong

		////////////////////////////////////////////////////////////////////////////

		return (jboolean)true;
	}
	catch(std::exception& e){ return (jboolean)false; }
}


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    invalidateMeasuredProgram
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_invalidateMeasuredProgram
  (JNIEnv * env, jobject obj)
{
	try{
		return engine.invalidateMeasuredProgram();
	}
	catch(std::exception& e){ return (jboolean)false; }
}

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getMeasuredProgram
 * Signature: ()[[F
 */
JNIEXPORT jobjectArray JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getMeasuredProgram
  (JNIEnv * env, jobject obj)
{
	try{
		std::vector< std::vector<float> > program;

		if(engine.getMeasuredProgram(program) == false){
			return NULL;
		}

		// creates dummy empty float array to get correct jclass
		jfloatArray test = (jfloatArray)env->NewFloatArray(0);
		jclass arrayClass = env->GetObjectClass(test);
		env->DeleteLocalRef(test);

		jobjectArray ret = (jobjectArray)env->NewObjectArray(program.size(),
				arrayClass, NULL);

		if(ret == NULL) return ret; // there was an error but there is nothing we can do about it..


		for(unsigned int i=0;i<program.size();i++){
			jfloatArray p = (jfloatArray)env->NewFloatArray(program[i].size());
			if(p == NULL) return ret;

			env->SetFloatArrayRegion(p, 0, program[i].size(), &(program[i][0]));

			env->SetObjectArrayElement(ret, i, p);
		}

		return ret;

	}
	catch(std::exception& e){ return NULL; }
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


/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    setParameter
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_setParameter
  (JNIEnv * env, jobject obj, jstring parameter, jstring value)
{
	try{
		if(env->IsSameObject(parameter, NULL)) return (jboolean)false;
		if(env->IsSameObject(value, NULL))     return (jboolean)false;

		const char *p = env->GetStringUTFChars(parameter, 0);
		const char* v = env->GetStringUTFChars(value, 0);

		bool result = engine.setParameter(std::string(p), std::string(v));

		env->ReleaseStringUTFChars(parameter, p);
		env->ReleaseStringUTFChars(value, v);

		return (jboolean)result;

	}
	catch(std::exception& e){
		return (jboolean)false;
	}
}
