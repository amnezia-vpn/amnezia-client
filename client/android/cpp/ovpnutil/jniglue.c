#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <unistd.h>


#include "jniglue.h"

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
#ifndef NDEBUG
    __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "Loading openvpn native library $id$ compiled on "   __DATE__ " " __TIME__ );
#endif
    return JNI_VERSION_1_2;
}


void android_openvpn_log(int level,const char* prefix,const char* prefix_sep,const char* m1)
{
    __android_log_print(ANDROID_LOG_DEBUG,"openvpn","%s%s%s",prefix,prefix_sep,m1);
}


//! Hack to get the current installed ABI of the libraries. See also https://github.com/schwabe/ics-openvpn/issues/391
jstring Java_de_blinkt_openvpn_core_NativeUtils_getJNIAPI(JNIEnv *env, jclass jo)
{

    return (*env)->NewStringUTF(env, TARGET_ARCH_ABI);
}

jstring Java_de_blinkt_openvpn_core_NativeUtils_getOpenVPN2GitVersion(JNIEnv *env, jclass jo)
{

  return (*env)->NewStringUTF(env, OPENVPN2_GIT_REVISION);
}

jstring Java_de_blinkt_openvpn_core_NativeUtils_getOpenVPN3GitVersion(JNIEnv *env, jclass jo)
{

  return (*env)->NewStringUTF(env, OPENVPN3_GIT_REVISION);
}
