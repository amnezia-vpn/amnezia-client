#include <QAndroidBinder>
#include <QAndroidIntent>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QAndroidParcel>
#include <QAndroidServiceConnection>
#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTextCodec>
#include <QTimer>
#include <QtAndroid>

#include "android_controller.h"
#include "core/errorstrings.h"
#include "ui/pages_logic/StartPageLogic.h"

// Binder Codes for VPNServiceBinder
// See also - VPNServiceBinder.kt
// Actions that are Requestable
const int ACTION_ACTIVATE = 1;
const int ACTION_DEACTIVATE = 2;
const int ACTION_REGISTER_LISTENER = 3;
const int ACTION_REQUEST_STATISTIC = 4;
const int ACTION_REQUEST_GET_LOG = 5;
const int ACTION_REQUEST_CLEANUP_LOG = 6;
const int ACTION_RESUME_ACTIVATE = 7;
const int ACTION_SET_NOTIFICATION_TEXT = 8;
const int ACTION_SET_NOTIFICATION_FALLBACK = 9;
const int ACTION_SHARE_CONFIG = 10;

// Event Types that will be Dispatched after registration
const int EVENT_INIT = 0;
const int EVENT_CONNECTED = 1;
const int EVENT_DISCONNECTED = 2;
const int EVENT_STATISTIC_UPDATE = 3;
const int EVENT_BACKEND_LOGS = 4;
const int EVENT_ACTIVATION_ERROR = 5;
const int EVENT_CONFIG_IMPORT = 6;

namespace {
AndroidController* s_instance = nullptr;

constexpr auto PERMISSIONHELPER_CLASS =
    "org/amnezia/vpn/qt/VPNPermissionHelper";

}  // namespace

AndroidController::AndroidController():
      m_binder(this)
{
    connect(this, &AndroidController::scheduleStatusCheckSignal, this, &AndroidController::scheduleStatusCheckSlot);
}

AndroidController* AndroidController::instance() {
    if (!s_instance) s_instance = new AndroidController();
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
    QAndroidJniObject javaClass(PERMISSIONHELPER_CLASS);
    QAndroidJniEnvironment env;
    jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
    env->RegisterNatives(objectClass, methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(objectClass);

    auto appContext = QtAndroid::androidActivity().callObjectMethod(
        "getApplicationContext", "()Landroid/content/Context;");

    QAndroidJniObject::callStaticMethod<void>(
        "org/amnezia/vpn/VPNService", "startService",
        "(Landroid/content/Context;)V", appContext.object());

    // Start the VPN Service (if not yet) and Bind to it
    const bool bindResult = QtAndroid::bindService(
        QAndroidIntent(appContext.object(), "org.amnezia.vpn.VPNService"),
        *this, QtAndroid::BindFlag::AutoCreate);
    qDebug() << "Binding to the service..." << bindResult;

    return bindResult;
}

ErrorCode AndroidController::start()
{

    //qDebug().noquote() << "AndroidController::start" << QJsonDocument(m_rawConfig).toJson();
    qDebug() << "Prompting for VPN permission";
    auto appContext = QtAndroid::androidActivity().callObjectMethod(
                "getApplicationContext", "()Landroid/content/Context;");
    QAndroidJniObject::callStaticMethod<void>(
                PERMISSIONHELPER_CLASS, "startService", "(Landroid/content/Context;)V",
                appContext.object());

    QAndroidParcel sendData;
    sendData.writeData(QJsonDocument(m_vpnConfig).toJson());
    bool activateResult = false;
    while (!activateResult){
        activateResult = m_serviceBinder.transact(ACTION_ACTIVATE, sendData, nullptr);
    }

    return activateResult ? NoError : UnknownError;
}

void AndroidController::stop() {
    qDebug() << "AndroidController::stop";

//  if (reason != ReasonNone) {
//    // Just show that we're disconnected
//    // we're doing the actual disconnect once
//    // the vpn-service has the new server ready in Action->Activate
//    emit disconnected();
//    qCritical() << "deactivation skipped for Switching";
//    return;
//  }

    QAndroidParcel nullData;
    m_serviceBinder.transact(ACTION_DEACTIVATE, nullData, nullptr);
}

// Activates the tunnel that is currently set
// in the VPN Service
void AndroidController::resumeStart() {
  QAndroidParcel nullData;
  m_serviceBinder.transact(ACTION_RESUME_ACTIVATE, nullData, nullptr);
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
  QAndroidParcel data;
  data.writeData(doc.toJson());
  m_serviceBinder.transact(ACTION_SET_NOTIFICATION_TEXT, data, nullptr);
}

void AndroidController::shareConfig(const QString& configContent, const QString& suggestedName) {
    QJsonObject rootObject;
    rootObject["data"] = configContent;
    rootObject["suggestedName"] = suggestedName;
    QJsonDocument doc(rootObject);
    QAndroidParcel parcel;
    parcel.writeData(doc.toJson());
    m_serviceBinder.transact(ACTION_SHARE_CONFIG, parcel, nullptr);
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
  QAndroidParcel data;
  data.writeData(doc.toJson());
  m_serviceBinder.transact(ACTION_SET_NOTIFICATION_FALLBACK, data, nullptr);
}

void AndroidController::checkStatus() {
  qDebug() << "check status";

  QAndroidParcel nullParcel;
  m_serviceBinder.transact(ACTION_REQUEST_STATISTIC, nullParcel, nullptr);
}

void AndroidController::getBackendLogs(std::function<void(const QString&)>&& a_callback) {
  qDebug() << "get logs";

  m_logCallback = std::move(a_callback);
  QAndroidParcel nullData, replyData;
  m_serviceBinder.transact(ACTION_REQUEST_GET_LOG, nullData, &replyData);
}

void AndroidController::cleanupBackendLogs() {
  qDebug() << "cleanup logs";

  QAndroidParcel nullParcel;
  m_serviceBinder.transact(ACTION_REQUEST_CLEANUP_LOG, nullParcel, nullptr);
}

void AndroidController::importConfig(const QString& data){
    m_startPageLogic->importConnectionFromCode(data);
}

void AndroidController::onServiceConnected(
    const QString& name, const QAndroidBinder& serviceBinder) {
  qDebug() << "Server " + name + " connected";

  Q_UNUSED(name);

  m_serviceBinder = serviceBinder;

  // Send the Service our Binder to recive incoming Events
  QAndroidParcel binderParcel;
  binderParcel.writeBinder(m_binder);
  m_serviceBinder.transact(ACTION_REGISTER_LISTENER, binderParcel, nullptr);
}

void AndroidController::onServiceDisconnected(const QString& name) {
  qDebug() << "Server disconnected";
  m_serviceConnected = false;
  Q_UNUSED(name);
  // TODO: Maybe restart? Or crash?
}

const QJsonObject &AndroidController::vpnConfig() const
{
    return m_vpnConfig;
}

void AndroidController::setVpnConfig(const QJsonObject &newVpnConfig)
{
    m_vpnConfig = newVpnConfig;
}

void AndroidController::scheduleStatusCheckSlot()
{
    QTimer::singleShot(1000, [this]() {
        if (isConnected) {
            checkStatus();
        }
    });
}

/**
 * @brief AndroidController::VPNBinder::onTransact
 * @param code the Event-Type we get From the VPNService See
 * @param data - Might contain UTF-8 JSON in case the Event has a payload
 * @param reply - always null
 * @param flags - unused
 * @return Returns true is the code was a valid Event Code
 */
bool AndroidController::VPNBinder::onTransact(int code,
                                              const QAndroidParcel& data,
                                              const QAndroidParcel& reply,
                                              QAndroidBinder::CallType flags) {
  Q_UNUSED(data);
  Q_UNUSED(reply);
  Q_UNUSED(flags);

  QJsonDocument doc;
  QString buffer;
  switch (code) {
    case EVENT_INIT:
      qDebug() << "Transact: init";
      doc = QJsonDocument::fromJson(data.readData());
      emit m_controller->initialized(
          true, doc.object()["connected"].toBool(),
          QDateTime::fromMSecsSinceEpoch(
              doc.object()["time"].toVariant().toLongLong()));
      // Pass a localised version of the Fallback string for the Notification
      m_controller->setFallbackConnectedNotification();

      break;
    case EVENT_CONNECTED:
      qDebug() << "Transact: connected";
      emit m_controller->connectionStateChanged(VpnProtocol::Connected);
      m_controller->isConnected = true;
      emit m_controller->scheduleStatusCheckSignal();
      break;
    case EVENT_DISCONNECTED:
      qDebug() << "Transact: disconnected";
      emit m_controller->connectionStateChanged(VpnProtocol::Disconnected);
      m_controller->isConnected = false;
      break;
    case EVENT_STATISTIC_UPDATE:
    {
      qDebug() << "Transact:: update";

      doc = QJsonDocument::fromJson(data.readData());

      QString rx = doc.object()["rx_bytes"].toString();
      QString tx = doc.object()["tx_bytes"].toString();
      QString endpoint = doc.object()["endpoint"].toString();
      QString deviceIPv4 = doc.object()["deviceIpv4"].toString();

      emit m_controller->statusUpdated(rx, tx, endpoint, deviceIPv4);
      emit m_controller->scheduleStatusCheckSignal();

      break;
    }
    case EVENT_BACKEND_LOGS:
      qDebug() << "Transact: backend logs";

      buffer = readUTF8Parcel(data);
      if (m_controller->m_logCallback) {
        m_controller->m_logCallback(buffer);
      }
      break;
    case EVENT_ACTIVATION_ERROR:
      qDebug() << "Transact: error";
      emit m_controller->connectionStateChanged(VpnProtocol::Error);
      break;
    case EVENT_CONFIG_IMPORT:
      qDebug() << "Transact: config import";
      doc = QJsonDocument::fromJson(data.readData());
      buffer = doc.object()["config"].toString();
      qDebug() << "Transact: config string" << buffer;
      m_controller->importConfig(buffer);
      break;
    default:
      qWarning() << "Transact: Invalid!";
      break;
  }

  return true;
}

QString AndroidController::VPNBinder::readUTF8Parcel(QAndroidParcel data) {
  // 106 is the Code for UTF-8
  return QTextCodec::codecForMib(106)->toUnicode(data.readData());
}

const int ACTIVITY_RESULT_OK = 0xffffffff;
/**
 * @brief Starts the Given intent in Context of the QTActivity
 * @param env
 * @param intent
 */
void AndroidController::startActivityForResult(JNIEnv *env, jobject, jobject intent)
{
    qDebug() << "start activity";
    Q_UNUSED(env);
    QtAndroid::startActivity(intent, 1337,
                             [](int receiverRequestCode, int resultCode,
                                const QAndroidJniObject& data) {
                               // Currently this function just used in
                               // VPNService.kt::checkPersmissions. So the result
                               // we're getting is if the User gave us the
                               // Vpn.bind permission. In case of NO we should
                               // abort.
                               Q_UNUSED(receiverRequestCode);
                               Q_UNUSED(data);

                               AndroidController* controller =
                                   AndroidController::instance();
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
