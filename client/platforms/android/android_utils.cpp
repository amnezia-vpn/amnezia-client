#include <QCoreApplication>
#include "android_utils.h"

namespace AndroidUtils
{

QJniObject getActivity()
{
    return QNativeInterface::QAndroidApplication::context();
}

QString convertJString(JNIEnv *env, jstring data)
{
    int len = env->GetStringLength(data);
    QString res(len, Qt::Uninitialized);
    env->GetStringRegion(data, 0, len, reinterpret_cast<jchar *>(res.data()));
    return res;
}

void runOnAndroidThreadSync(const std::function<void()> &runnable)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread(runnable).waitForFinished();
}

void runOnAndroidThreadAsync(const std::function<void()> &runnable)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread(runnable);
}

}
