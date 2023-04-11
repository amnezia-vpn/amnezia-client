/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTextCodec>
#include <QTimer>

#include "android_controller.h"
#include "private/qandroidextras_p.h"
#include "ui/pages_logic/StartPageLogic.h"

#include "androidvpnactivity.h"
#include "androidutils.h"

namespace {
AndroidController* s_instance = nullptr;

constexpr auto PERMISSIONHELPER_CLASS =
    "org/amnezia/vpn/qt/VPNPermissionHelper";
}  // namespace

AndroidController::AndroidController() : QObject()
{
    connect(this, &AndroidController::scheduleStatusCheckSignal, this, &AndroidController::scheduleStatusCheckSlot);

    s_instance = this;

    auto activity = AndroidVPNActivity::instance();

    connect(activity, &AndroidVPNActivity::serviceConnected, this, []() {
        qDebug() << "Transact: service connected";
        AndroidVPNActivity::sendToService(ServiceAction::ACTION_REQUEST_STATISTIC, "");
    }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventInitialized, this,
        [this](const QString& parcelBody) {
            // We might get multiple Init events as widgets, or fragments
            // might query this.
            if (m_init) {
                return;
            }

            qDebug() << "Transact: init";

            m_init = true;

            auto doc = QJsonDocument::fromJson(parcelBody.toUtf8());
            qlonglong time = doc.object()["time"].toVariant().toLongLong();

            isConnected = doc.object()["connected"].toBool();

            if (isConnected) {
                emit scheduleStatusCheckSignal();
            }

            emit initialized(
                true, isConnected,
                time > 0 ? QDateTime::fromMSecsSinceEpoch(time) : QDateTime());

            setFallbackConnectedNotification();
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventConnected, this,
        [this](const QString& parcelBody) {
            Q_UNUSED(parcelBody);
            qDebug() << "Transact: connected";

            if (!isConnected) {
                emit scheduleStatusCheckSignal();
            }

            isConnected = true;

            emit connectionStateChanged(VpnProtocol::Connected);
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventDisconnected, this,
        [this]() {
            qDebug() << "Transact: disconnected";

            isConnected = false;

            emit connectionStateChanged(VpnProtocol::Disconnected);
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventStatisticUpdate, this,
        [this](const QString& parcelBody) {
            qDebug() << "Transact: update";
            auto doc = QJsonDocument::fromJson(parcelBody.toUtf8());

            QString rx = doc.object()["rx_bytes"].toString();
            QString tx = doc.object()["tx_bytes"].toString();
            QString endpoint = doc.object()["endpoint"].toString();
            QString deviceIPv4 = doc.object()["deviceIpv4"].toString();

            emit statusUpdated(rx, tx, endpoint, deviceIPv4);
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventBackendLogs, this,
        [this](const QString& parcelBody) {
            qDebug() << "Transact: backend logs";

            QString buffer = parcelBody.toUtf8();
            if (m_logCallback) {
                m_logCallback(buffer);
            }
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventActivationError, this,
        [this](const QString& parcelBody) {
            Q_UNUSED(parcelBody)
            qDebug() << "Transact: error";
            emit connectionStateChanged(VpnProtocol::Error);
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::eventConfigImport, this,
        [this](const QString& parcelBody) {
            qDebug() << "Transact: config import";
            auto doc = QJsonDocument::fromJson(parcelBody.toUtf8());

            QString buffer = doc.object()["config"].toString();
            qDebug() << "Transact: config string" << buffer;
            importConfig(buffer);
        }, Qt::QueuedConnection);

    connect(activity, &AndroidVPNActivity::serviceDisconnected, this,
        [this]() {
            qDebug() << "Transact: service disconnected";
            m_serviceConnected = false;
        }, Qt::QueuedConnection);
}

AndroidController* AndroidController::instance() {
    if (!s_instance) {
        s_instance = new AndroidController();
    }

    return s_instance;
}

bool AndroidController::initialize(StartPageLogic *startPageLogic)
{
    qDebug() << "Initializing";

    m_startPageLogic = startPageLogic;

    // Hook in the native implementation for startActivityForResult into the JNI
    JNINativeMethod methods[]{{"startActivityForResult",
                               "(Landroid/content/Intent;)V",
                               reinterpret_cast<void*>(startActivityForResult)}};
    QJniObject javaClass(PERMISSIONHELPER_CLASS);
    QJniEnvironment env;
    jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
    env->RegisterNatives(objectClass, methods, sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(objectClass);

    AndroidVPNActivity::connectService();

    return true;
}

ErrorCode AndroidController::start()
{
    qDebug() << "Prompting for VPN permission";
    QJniObject activity = AndroidUtils::getActivity();
    auto appContext = activity.callObjectMethod(
                "getApplicationContext", "()Landroid/content/Context;");
    QJniObject::callStaticMethod<void>(
                PERMISSIONHELPER_CLASS, "startService", "(Landroid/content/Context;)V",
                appContext.object());

    QJsonDocument doc(m_vpnConfig);
    AndroidVPNActivity::sendToService(ServiceAction::ACTION_ACTIVATE, doc.toJson());

    return NoError;
}

void AndroidController::stop() {
    qDebug() << "AndroidController::stop";

    AndroidVPNActivity::sendToService(ServiceAction::ACTION_DEACTIVATE, QString());
}

// Activates the tunnel that is currently set
// in the VPN Service
void AndroidController::resumeStart() {
    AndroidVPNActivity::sendToService(ServiceAction::ACTION_RESUME_ACTIVATE, QString());
}

/*
 * Sets the current notification text that is shown
 */
void AndroidController::setNotificationText(const QString& title,
                                            const QString& message,
                                            int timerSec) {
    QJsonObject args;
    args["title"] = title;
    args["message"] = message;
    args["sec"] = timerSec;
    QJsonDocument doc(args);

    AndroidVPNActivity::sendToService(ServiceAction::ACTION_SET_NOTIFICATION_TEXT, doc.toJson());
}

void AndroidController::shareConfig(const QString& configContent, const QString& suggestedName) {
    AndroidVPNActivity::saveFileAs(configContent, suggestedName);
}

/*
 * Sets fallback Notification text that should be shown in case the VPN
 * switches into the Connected state without the app open
 * e.g via always-on vpn
 */
void AndroidController::setFallbackConnectedNotification() {
    QJsonObject args;
    args["title"] = tr("AmneziaVPN");
    //% "Ready for you to connect"
    //: Refers to the app - which is currently running the background and waiting
    args["message"] = tr("VPN Connected");
    QJsonDocument doc(args);

    AndroidVPNActivity::sendToService(ServiceAction::ACTION_SET_NOTIFICATION_FALLBACK, doc.toJson());
}

void AndroidController::checkStatus() {
    qDebug() << "check status";

    AndroidVPNActivity::sendToService(ServiceAction::ACTION_REQUEST_STATISTIC, QString());
}

void AndroidController::getBackendLogs(std::function<void(const QString&)>&& a_callback) {
    qDebug() << "get logs";

    m_logCallback = std::move(a_callback);

    AndroidVPNActivity::sendToService(ServiceAction::ACTION_REQUEST_GET_LOG, QString());
}

void AndroidController::cleanupBackendLogs() {
    qDebug() << "cleanup logs";

    AndroidVPNActivity::sendToService(ServiceAction::ACTION_REQUEST_CLEANUP_LOG, QString());
}

void AndroidController::importConfig(const QString& data){
    m_startPageLogic->selectConfigFormat(data);
}

const QJsonObject &AndroidController::vpnConfig() const
{
    return m_vpnConfig;
}

void AndroidController::setVpnConfig(const QJsonObject &newVpnConfig)
{
    m_vpnConfig = newVpnConfig;
}

void AndroidController::startQrReaderActivity()
{
    AndroidVPNActivity::instance()->startQrCodeReader();
}

void AndroidController::copyTextToClipboard(QString text)
{
    AndroidVPNActivity::instance()->copyTextToClipboard(text);
}

void AndroidController::scheduleStatusCheckSlot()
{
    QTimer::singleShot(1000, [this]() {
        if (isConnected) {
            checkStatus();
            emit scheduleStatusCheckSignal();
        }
    });
}

const int ACTIVITY_RESULT_OK = 0xffffffff;
/**
 * @brief Starts the Given intent in Context of the QTActivity
 * @param env
 * @param intent
 */
void AndroidController::startActivityForResult(JNIEnv *env, jobject, jobject intent)
{
    qDebug() << "start vpnPermissionHelper";
    Q_UNUSED(env);

    QtAndroidPrivate::startActivity(intent, 1337,
         [](int receiverRequestCode, int resultCode,
            const QJniObject& data) {
           // Currently this function just used in
           // VPNService.kt::checkPermissions. So the result
           // we're getting is if the User gave us the
           // Vpn.bind permission. In case of NO we should
           // abort.
           Q_UNUSED(receiverRequestCode);
           Q_UNUSED(data);

           AndroidController* controller = AndroidController::instance();
           if (!controller) {
                return;
           }

           if (resultCode == ACTIVITY_RESULT_OK) {
                qDebug() << "VPN PROMPT RESULT - Accepted";
                controller->resumeStart();
                return;
           }
           // If the request got rejected abort the current
           // connection.
           qWarning() << "VPN PROMPT RESULT - Rejected";
           emit controller->connectionStateChanged(VpnProtocol::Disconnected);
         });
    return;
}
