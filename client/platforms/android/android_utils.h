#ifndef ANDROID_UTILS_H
#define ANDROID_UTILS_H

#include <QJniObject>

namespace AndroidUtils
{
QJniObject getActivity();

QString convertJString(JNIEnv *env, jstring data);

void runOnAndroidThreadSync(const std::function<void()> &runnable);
void runOnAndroidThreadAsync(const std::function<void()> &runnable);
};

#endif // ANDROID_UTILS_H
