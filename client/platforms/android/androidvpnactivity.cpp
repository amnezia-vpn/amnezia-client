/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "androidvpnactivity.h"

#include <QApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QJsonDocument>
#include <QJsonObject>

#include "androidutils.h"
#include "jni.h"

namespace {
    AndroidVPNActivity* s_instance = nullptr;
    constexpr auto CLASSNAME = "org.amnezia.vpn.qt.VPNActivity";
}

AndroidVPNActivity::AndroidVPNActivity() {
    AndroidUtils::runOnAndroidThreadAsync([]() {
        JNINativeMethod methods[]{
            {"handleBackButton", "()Z", reinterpret_cast<bool*>(handleBackButton)},
            {"onServiceMessage", "(ILjava/lang/String;)V",
             reinterpret_cast<void*>(onServiceMessage)},
            {"qtOnServiceConnected", "()V",
             reinterpret_cast<void*>(onServiceConnected)},
            {"qtOnServiceDisconnected", "()V",
             reinterpret_cast<void*>(onServiceDisconnected)},
        };

        QJniObject javaClass(CLASSNAME);
        QJniEnvironment env;
        jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
        env->RegisterNatives(objectClass, methods, sizeof(methods) / sizeof(methods[0]));
        env->DeleteLocalRef(objectClass);
    });
}

void AndroidVPNActivity::maybeInit() {
    if (s_instance == nullptr) {
        s_instance = new AndroidVPNActivity();
    }
}

// static
bool AndroidVPNActivity::handleBackButton(JNIEnv* env, jobject thiz) {
    Q_UNUSED(env);
    Q_UNUSED(thiz);
}

void AndroidVPNActivity::connectService() {
    QJniObject::callStaticMethod<void>(CLASSNAME, "connectService", "()V");
}

// static
AndroidVPNActivity* AndroidVPNActivity::instance() {
    if (s_instance == nullptr) {
        AndroidVPNActivity::maybeInit();
    }

    return s_instance;
}

// static
void AndroidVPNActivity::sendToService(ServiceAction type, const QString& data) {
    int messageType = (int)type;

    QJniEnvironment env;
    QJniObject::callStaticMethod<void>(
        CLASSNAME, "sendToService", "(ILjava/lang/String;)V",
        static_cast<int>(messageType),
        QJniObject::fromString(data).object<jstring>());
}

// static
void AndroidVPNActivity::onServiceMessage(JNIEnv* env, jobject thiz,
                                          jint messageType, jstring body) {
    Q_UNUSED(thiz);
    const char* buffer = env->GetStringUTFChars(body, nullptr);
    if (!buffer) {
        return;
    }

    QString parcelBody(buffer);
    env->ReleaseStringUTFChars(body, buffer);
    AndroidUtils::dispatchToMainThread([messageType, parcelBody] {
        AndroidVPNActivity::instance()->handleServiceMessage(messageType,
                                                            parcelBody);
    });
}

void AndroidVPNActivity::handleServiceMessage(int code, const QString& data) {
    auto mode = (ServiceEvents)code;

    switch (mode) {
    case ServiceEvents::EVENT_INIT:
        emit eventInitialized(data);
        break;
    case ServiceEvents::EVENT_CONNECTED:
        emit eventConnected(data);
        break;
    case ServiceEvents::EVENT_DISCONNECTED:
        emit eventDisconnected(data);
        break;
    case ServiceEvents::EVENT_STATISTIC_UPDATE:
        emit eventStatisticUpdate(data);
        break;
    case ServiceEvents::EVENT_BACKEND_LOGS:
        emit eventBackendLogs(data);
        break;
    case ServiceEvents::EVENT_ACTIVATION_ERROR:
        emit eventActivationError(data);
        break;
    case ServiceEvents::EVENT_CONFIG_IMPORT:
        emit eventConfigImport(data);
        break;
    default:
        Q_ASSERT(false);
    }
}

void AndroidVPNActivity::onServiceConnected(JNIEnv* env, jobject thiz) {
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidVPNActivity::instance()->serviceConnected();
}

void AndroidVPNActivity::onServiceDisconnected(JNIEnv* env, jobject thiz) {
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidVPNActivity::instance()->serviceDisconnected();
}
