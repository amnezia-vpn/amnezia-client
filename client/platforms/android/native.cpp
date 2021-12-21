#include "native.h"
#include <QMetaObject>
#if defined(Q_OS_ANDROID)
    #include <jni.h>
#endif // Q_OS_ANDROID


QObject *NativeHelpers::application_p_ = 0;

#if defined(Q_OS_ANDROID)

// define our native static functions
// these are the functions that Java part will call directly from Android UI thread
static void onPermissionsGranted(JNIEnv * /*env*/, jobject /*obj*/)
{
    QMetaObject::invokeMethod(NativeHelpers::getApplicationInstance(), "onPermissionsGranted"
                              , Qt::QueuedConnection);
}

static void onPermissionsDenied(JNIEnv * /*env*/, jobject /*obj*/)
{
    QMetaObject::invokeMethod(NativeHelpers::getApplicationInstance(), "onPermissionsDenied"
                              , Qt::QueuedConnection);
}

//create a vector with all our JNINativeMethod(s)
static JNINativeMethod methods[] = {
    {"onPermissionsGranted", "()V", (void *)onPermissionsGranted},
    {"onPermissionsDenied", "()V", (void *)onPermissionsDenied},
};

// this method is called automatically by Java after the .so file is loaded
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    JNIEnv* env;
    // get the JNIEnv pointer.
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
      return JNI_ERR;

    // search for Java class which declares the native methods
    jclass javaClass = env->FindClass("org/ftylitak/qzxing/NativeFunctions");
    if (!javaClass)
      return JNI_ERR;

    // register our native methods
    if (env->RegisterNatives(javaClass, methods,
                          sizeof(methods) / sizeof(methods[0])) < 0) {
      return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

#endif // Q_OS_ANDROID
