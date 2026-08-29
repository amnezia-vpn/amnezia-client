#include "jni_helper.h"

// Cached handles to org.amnezia.vpn.protocol.dnstt.DnsttNative. They are
// resolved in JNI_OnLoad, which runs on a thread whose class loader can see
// application classes; FindClass from a Go-created thread could not.
static JavaVM   *g_dnstt_vm      = NULL;
static jclass    g_dnstt_class   = NULL;
static jmethodID g_dnstt_protect = NULL;
static jmethodID g_dnstt_log     = NULL;
static jmethodID g_dnstt_state   = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jclass cls = NULL;

    (void)reserved;
    g_dnstt_vm = vm;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_VERSION_1_6;
    }

    cls = (*env)->FindClass(env, "org/amnezia/vpn/protocol/dnstt/DnsttNative");
    if (cls == NULL) {
        (*env)->ExceptionClear(env);
        return JNI_VERSION_1_6;
    }

    g_dnstt_class = (jclass)(*env)->NewGlobalRef(env, cls);
    g_dnstt_protect = (*env)->GetStaticMethodID(env, g_dnstt_class, "protectSocket", "(I)Z");
    if (g_dnstt_protect == NULL) {
        (*env)->ExceptionClear(env);
    }
    g_dnstt_log = (*env)->GetStaticMethodID(env, g_dnstt_class, "nativeLog", "(Ljava/lang/String;)V");
    if (g_dnstt_log == NULL) {
        (*env)->ExceptionClear(env);
    }
    g_dnstt_state = (*env)->GetStaticMethodID(env, g_dnstt_class, "onStateChanged", "(Ljava/lang/String;)V");
    if (g_dnstt_state == NULL) {
        (*env)->ExceptionClear(env);
    }

    return JNI_VERSION_1_6;
}

// _attachEnv returns a JNIEnv for the calling thread, attaching it to the JVM
// if needed. *attached is set when the caller must detach afterwards.
static JNIEnv* _attachEnv(int *attached) {
    JNIEnv *env = NULL;
    *attached = 0;
    if (g_dnstt_vm == NULL) return NULL;
    if ((*g_dnstt_vm)->GetEnv(g_dnstt_vm, (void **)&env, JNI_VERSION_1_6) == JNI_OK) {
        return env;
    }
    if ((*g_dnstt_vm)->AttachCurrentThread(g_dnstt_vm, &env, NULL) != 0) {
        return NULL;
    }
    *attached = 1;
    return env;
}

static void _detachEnv(int attached) {
    if (attached && g_dnstt_vm != NULL) {
        (*g_dnstt_vm)->DetachCurrentThread(g_dnstt_vm);
    }
}

// _protectSocket calls VpnService.protect via DnsttNative.protectSocket. It is
// invoked from Go-owned threads, hence the attach/detach dance.
int _protectSocket(int fd) {
    int attached = 0;
    jboolean result = JNI_FALSE;
    JNIEnv *env = NULL;

    if (g_dnstt_class == NULL || g_dnstt_protect == NULL) return 0;
    env = _attachEnv(&attached);
    if (env == NULL) return 0;

    result = (*env)->CallStaticBooleanMethod(env, g_dnstt_class, g_dnstt_protect, (jint)fd);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        result = JNI_FALSE;
    }

    _detachEnv(attached);
    return result == JNI_TRUE ? 1 : 0;
}

// _nativeLog forwards a Go log line to android.util.Log on the Kotlin side.
void _nativeLog(const char *msg) {
    int attached = 0;
    JNIEnv *env = NULL;
    jstring jmsg = NULL;

    if (msg == NULL || g_dnstt_class == NULL || g_dnstt_log == NULL) return;
    env = _attachEnv(&attached);
    if (env == NULL) return;

    jmsg = (*env)->NewStringUTF(env, msg);
    if (jmsg != NULL) {
        (*env)->CallStaticVoidMethod(env, g_dnstt_class, g_dnstt_log, jmsg);
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        (*env)->DeleteLocalRef(env, jmsg);
    }

    _detachEnv(attached);
}

// _notifyState reports a tunnel state transition to the Kotlin layer so the UI
// stops claiming "connected" while the session is being rebuilt.
void _notifyState(const char *state) {
    int attached = 0;
    JNIEnv *env = NULL;
    jstring jstate = NULL;

    if (state == NULL || g_dnstt_class == NULL || g_dnstt_state == NULL) return;
    env = _attachEnv(&attached);
    if (env == NULL) return;

    jstate = (*env)->NewStringUTF(env, state);
    if (jstate != NULL) {
        (*env)->CallStaticVoidMethod(env, g_dnstt_class, g_dnstt_state, jstate);
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        }
        (*env)->DeleteLocalRef(env, jstate);
    }

    _detachEnv(attached);
}
