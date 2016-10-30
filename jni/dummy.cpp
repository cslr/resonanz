// DUMMY JNI SHARED LIBRARY FILE USED FOR TESTING!


#include "fi_iki_nop_neuromancer_ResonanzEngine.h"
#include <jni.h>
#include <exception>

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startRandomStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startRandomStimulation
(JNIEnv *env, jobject o , jstring s1, jstring s2){ return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startMeasureStimulation
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureStimulation
(JNIEnv *, jobject, jstring, jstring, jstring){ return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startOptimizeModel
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startOptimizeModel
(JNIEnv *, jobject, jstring, jstring, jstring){ return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startExecuteProgram
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[[FZZ)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startExecuteProgram
(JNIEnv *, jobject, jstring, jstring, jstring, jstring, jobjectArray, jobjectArray, jboolean, jboolean)
{ return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    startMeasureProgram
 * Signature: (Ljava/lang/String;[Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_startMeasureProgram
(JNIEnv *, jobject, jstring, jobjectArray, jint) { return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    invalidateMeasuredProgram
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_invalidateMeasuredProgram
(JNIEnv *, jobject) { return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getMeasuredProgram
 * Signature: ()[[F
 */
JNIEXPORT jobjectArray JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getMeasuredProgram
(JNIEnv *, jobject) { return NULL; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    stopCommand
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_stopCommand
(JNIEnv *, jobject) { return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    isBusy
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_isBusy
(JNIEnv *, jobject) { return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getStatusLine
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getStatusLine
(JNIEnv *, jobject) { return NULL; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getAnalyzeModel
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getAnalyzeModel
(JNIEnv *, jobject, jstring) { return NULL; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getDeltaStatistics
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getDeltaStatistics
(JNIEnv *, jobject, jstring, jstring, jstring) { return NULL; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getLastExecutedProgramStatisics
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getLastExecutedProgramStatisics
(JNIEnv *, jobject) { return NULL; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    deleteModelData
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_deleteModelData
(JNIEnv *, jobject, jstring) { return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    setEEGSourceDevice
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_setEEGSourceDevice
(JNIEnv *, jobject, jint) { return true; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getEEGSourceDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getEEGSourceDevice
(JNIEnv *, jobject) { return 0; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    getEEGDeviceStatus
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_getEEGDeviceStatus
(JNIEnv *, jobject){ return NULL; }

/*
 * Class:     fi_iki_nop_neuromancer_ResonanzEngine
 * Method:    setParameter
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_fi_iki_nop_neuromancer_ResonanzEngine_setParameter
(JNIEnv *, jobject, jstring, jstring) { return true; }

