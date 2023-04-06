/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ANDROIDUTILS_H
#define ANDROIDUTILS_H

#include <jni.h>

#include <QJniEnvironment>
#include <QJniObject>
#include <QObject>
#include <QString>
#include <QUrl>

class AndroidUtils final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AndroidUtils)

public:
    static QString GetDeviceName();

    static int GetSDKVersion();
    static QString GetManufacturer();

    static AndroidUtils* instance();

    static void dispatchToMainThread(std::function<void()> callback);

    static QByteArray getQByteArrayFromJString(JNIEnv* env, jstring data);

    static jbyteArray tojByteArray(const QByteArray& data);

    static QString getQStringFromJString(JNIEnv* env, jstring data);

    static QJsonObject getQJsonObjectFromJString(JNIEnv* env, jstring data);

    static QJniObject getActivity();

    static void runOnAndroidThreadSync(const std::function<void()> runnable);
    static void runOnAndroidThreadAsync(const std::function<void()> runnable);

private:
    AndroidUtils(QObject* parent);
    ~AndroidUtils();
};

#endif // ANDROIDUTILS_H
