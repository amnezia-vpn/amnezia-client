#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJsonDocument>

#include "android_controller.h"
#include "ui/controllers/importController.h"

namespace
{
    AndroidController *s_instance = nullptr;

    constexpr auto QT_ANDROID_CONTROLLER_CLASS = "org/amnezia/vpn/qt/QtAndroidController";
} // namespace

AndroidController::AndroidController() : QObject()
{
    connect(this, &AndroidController::status, this,
            [this](bool isVpnConnected) {
                qDebug() << "Android event: status; connected:" << isVpnConnected;
                if (isWaitingStatus) {
                    qDebug() << "Android VPN service is alive, initialization by service status";
                    isWaitingStatus = false;
                    emit serviceIsAlive(isVpnConnected);
                }
            },
            Qt::QueuedConnection);

    connect(
        this, &AndroidController::serviceDisconnected, this,
        [this]() {
            qDebug() << "Android event: service disconnected";
            isWaitingStatus = true;
            emit connectionStateChanged(Vpn::ConnectionState::Unknown);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::serviceError, this,
        [this]() {
            qDebug() << "Android event: service error";
            // todo: add error message
            emit connectionStateChanged(Vpn::ConnectionState::Error);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnPermissionRejected, this,
        [this]() {
            qWarning() << "Android event: VPN permission rejected";
            emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnConnected, this,
        [this]() {
            qDebug() << "Android event: VPN connected";
            emit connectionStateChanged(Vpn::ConnectionState::Connected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnDisconnected, this,
        [this]() {
            qDebug() << "Android event: VPN disconnected";
            emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::configImported, this,
        []() {
            // todo: not yet implemented
            qDebug() << "Transact: config import";
            /*auto doc = QJsonDocument::fromJson(parcelBody.toUtf8());

            QString buffer = doc.object()["config"].toString();
            qDebug() << "Transact: config string" << buffer;
            importConfigFromOutside(buffer);*/
        },
        Qt::QueuedConnection);
}

AndroidController *AndroidController::instance()
{
    if (!s_instance) {
        s_instance = new AndroidController();
    }

    return s_instance;
}

bool AndroidController::initialize()
{
    qDebug() << "Initialize AndroidController";

    const JNINativeMethod methods[] = {
        {"onStatus", "(Z)V", reinterpret_cast<void *>(onStatus)},
        {"onServiceDisconnected", "()V", reinterpret_cast<void *>(onServiceDisconnected)},
        {"onServiceError", "()V", reinterpret_cast<void *>(onServiceError)},
        {"onVpnPermissionRejected", "()V", reinterpret_cast<void *>(onVpnPermissionRejected)},
        {"onVpnConnected", "()V", reinterpret_cast<void *>(onVpnConnected)},
        {"onVpnDisconnected", "()V", reinterpret_cast<void *>(onVpnDisconnected)},
        {"onStatisticsUpdate", "(JJ)V", reinterpret_cast<void *>(onStatisticsUpdate)},
        {"onConfigImported", "()V", reinterpret_cast<void *>(onConfigImported)},
        {"decodeQrCode", "(Ljava/lang/String;)Z", reinterpret_cast<bool *>(decodeQrCode)}
    };

    QJniEnvironment env;
    bool registered = env.registerNativeMethods(QT_ANDROID_CONTROLLER_CLASS, methods,
                                                sizeof(methods) / sizeof(JNINativeMethod));
    if (!registered) {
        qCritical() << "Failed native method registration";
        return false;
    }
    qtAndroidControllerInitialized();
    return true;
}

// static
template <typename Ret, typename ...Args>
auto AndroidController::callActivityMethod(const char *methodName, const char *signature,
                                           const std::function<Ret()> &defValue, Args &&...args)
{
    qDebug() << "Call activity method:" << methodName;
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        return activity.callMethod<Ret>(methodName, signature, std::forward<Args>(args)...);
    } else {
        qCritical() << "Activity is not valid";
        return defValue();
    }
}

// static
template <typename ...Args>
void AndroidController::callActivityMethod(const char *methodName, const char *signature, Args &&...args)
{
    callActivityMethod<void>(methodName, signature, [] {}, std::forward<Args>(args)...);
}

ErrorCode AndroidController::start(const QJsonObject &vpnConfig)
{
    isWaitingStatus = false;
    auto config = QJsonDocument(vpnConfig).toJson();
    callActivityMethod("start", "(Ljava/lang/String;)V",
                       QJniObject::fromString(config).object<jstring>());

    return NoError;
}

void AndroidController::stop()
{
    callActivityMethod("stop", "()V");
}

void AndroidController::saveFile(const QString &fileName, const QString &data)
{
    callActivityMethod("saveFile", "(Ljava/lang/String;Ljava/lang/String;)V",
                       QJniObject::fromString(fileName).object<jstring>(),
                       QJniObject::fromString(data).object<jstring>());
}

void AndroidController::setNotificationText(const QString &title, const QString &message, int timerSec)
{
    callActivityMethod("setNotificationText", "(Ljava/lang/String;Ljava/lang/String;I)V",
                       QJniObject::fromString(title).object<jstring>(),
                       QJniObject::fromString(message).object<jstring>(),
                       (jint) timerSec);
}

void AndroidController::startQrReaderActivity()
{
    callActivityMethod("startQrCodeReader", "()V");
}

void AndroidController::qtAndroidControllerInitialized()
{
    callActivityMethod("qtAndroidControllerInitialized", "()V");
}

// JNI functions called by Android
// static
void AndroidController::onStatus(JNIEnv *env, jobject thiz, jboolean isVpnConnected)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->status(isVpnConnected);
}

// static
void AndroidController::onServiceDisconnected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->serviceDisconnected();
}

// static
void AndroidController::onServiceError(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->serviceError();
}

// static
void AndroidController::onVpnPermissionRejected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnPermissionRejected();
}

// static
void AndroidController::onVpnConnected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnConnected();
}

// static
void AndroidController::onVpnDisconnected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnDisconnected();
}

// static
void AndroidController::onStatisticsUpdate(JNIEnv *env, jobject thiz, jlong rxBytes, jlong txBytes)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->statisticsUpdated((quint64) rxBytes, (quint64) txBytes);
}

// static
void AndroidController::onConfigImported(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->configImported();
}

// static
bool AndroidController::decodeQrCode(JNIEnv *env, jobject thiz, jstring data)
{
    Q_UNUSED(thiz);

    const char *buffer = env->GetStringUTFChars(data, nullptr);
    if (!buffer) {
        return false;
    }

    QString code(buffer);
    env->ReleaseStringUTFChars(data, buffer);
    return ImportController::decodeQrCode(code);
}
