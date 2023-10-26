/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "androidutils.h"

#include <QGuiApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkCookieJar>
#include <QTimer>
#include <QUrlQuery>

#include "jni.h"

namespace
{
    AndroidUtils *s_instance = nullptr;
} // namespace

// static
QString AndroidUtils::GetDeviceName()
{
    QJniEnvironment env;
    jclass BUILD = env->FindClass("android/os/Build");
    jfieldID model = env->GetStaticFieldID(BUILD, "MODEL", "Ljava/lang/String;");
    jstring value = (jstring)env->GetStaticObjectField(BUILD, model);

    if (!value) {
        return QString("Android Device");
    }

    const char *buffer = env->GetStringUTFChars(value, nullptr);
    if (!buffer) {
        return QString("Android Device");
    }

    QString res(buffer);
    env->ReleaseStringUTFChars(value, buffer);

    return res;
};

// static
AndroidUtils *AndroidUtils::instance()
{
    if (!s_instance) {
        Q_ASSERT(qApp);
        s_instance = new AndroidUtils(qApp);
    }

    return s_instance;
}

AndroidUtils::AndroidUtils(QObject *parent) : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

AndroidUtils::~AndroidUtils()
{
    Q_ASSERT(s_instance == this);
    s_instance = nullptr;
}

// static
void AndroidUtils::dispatchToMainThread(std::function<void()> callback)
{
    QTimer *timer = new QTimer();
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]() {
        callback();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection);
}

// static
QByteArray AndroidUtils::getQByteArrayFromJString(JNIEnv *env, jstring data)
{
    const char *buffer = env->GetStringUTFChars(data, nullptr);
    if (!buffer) {
        qDebug() << "getQByteArrayFromJString - failed to parse data.";
        return QByteArray();
    }

    QByteArray out(buffer);
    env->ReleaseStringUTFChars(data, buffer);
    return out;
}

// static
QString AndroidUtils::getQStringFromJString(JNIEnv *env, jstring data)
{
    const char *buffer = env->GetStringUTFChars(data, nullptr);
    if (!buffer) {
        qDebug() << "getQStringFromJString - failed to parse data.";
        return QString();
    }

    QString out(buffer);
    env->ReleaseStringUTFChars(data, buffer);
    return out;
}

// static
QJsonObject AndroidUtils::getQJsonObjectFromJString(JNIEnv *env, jstring data)
{
    QByteArray raw(getQByteArrayFromJString(env, data));
    QJsonParseError jsonError;
    QJsonDocument json = QJsonDocument::fromJson(raw, &jsonError);
    if (QJsonParseError::NoError != jsonError.error) {
        qDebug() << "getQJsonObjectFromJstring - error parsing json. Code: " << jsonError.error
                 << "Offset: " << jsonError.offset << "Message: " << jsonError.errorString() << "Data: " << raw;
        return QJsonObject();
    }

    if (!json.isObject()) {
        qDebug() << "getQJsonObjectFromJString - object expected.";
        return QJsonObject();
    }

    return json.object();
}

QJniObject AndroidUtils::getActivity()
{
    return QNativeInterface::QAndroidApplication::context();
}

int AndroidUtils::GetSDKVersion()
{
    QJniEnvironment env;
    jclass versionClass = env->FindClass("android/os/Build$VERSION");
    jfieldID sdkIntFieldID = env->GetStaticFieldID(versionClass, "SDK_INT", "I");
    int sdk = env->GetStaticIntField(versionClass, sdkIntFieldID);

    return sdk;
}

QString AndroidUtils::GetManufacturer()
{
    QJniEnvironment env;
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID manuFacturerField = env->GetStaticFieldID(buildClass, "MANUFACTURER", "Ljava/lang/String;");
    jstring value = (jstring)env->GetStaticObjectField(buildClass, manuFacturerField);

    const char *buffer = env->GetStringUTFChars(value, nullptr);

    if (!buffer) {
        qDebug() << "Failed to fetch MANUFACTURER";
        return QByteArray();
    }

    QString res(buffer);
    qDebug() << "MANUFACTURER: " << res;
    env->ReleaseStringUTFChars(value, buffer);
    return res;
}

void AndroidUtils::runOnAndroidThreadSync(const std::function<void()> runnable)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread(runnable).waitForFinished();
}

void AndroidUtils::runOnAndroidThreadAsync(const std::function<void()> runnable)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread(runnable);
}

// Static
// Creates a copy of the passed QByteArray in the JVM and passes back a ref
jbyteArray AndroidUtils::tojByteArray(const QByteArray &data)
{
    QJniEnvironment env;
    jbyteArray out = env->NewByteArray(data.size());
    env->SetByteArrayRegion(out, 0, data.size(), reinterpret_cast<const jbyte *>(data.constData()));
    return out;
}
