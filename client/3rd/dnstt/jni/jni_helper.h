#ifndef JNI_HELPER_H
#define JNI_HELPER_H

#include <jni.h>
#include <stdlib.h>

// String helpers are inline: every cgo translation unit gets its own copy.
static inline const char* _getStringUTFChars(JNIEnv *env, jstring str, jboolean *isCopy) {
    if (!env || !str) return 0;
    return (*env)->GetStringUTFChars(env, str, isCopy);
}

static inline void _releaseStringUTFChars(JNIEnv *env, jstring str, const char *chars) {
    if (!env || !str || !chars) return;
    (*env)->ReleaseStringUTFChars(env, str, chars);
}

static inline jstring _newStringUTF(JNIEnv *env, const char *chars) {
    if (!env || !chars) return 0;
    return (*env)->NewStringUTF(env, chars);
}

// Defined once in jni_helper.c. JNI_OnLoad in particular must not be inlined
// into every translation unit, or the linker sees duplicate symbols.
int  _protectSocket(int fd);
void _nativeLog(const char *msg);
void _notifyState(const char *state);

#endif
