#ifndef ANDROID_CONTROLLER_H
#define ANDROID_CONTROLLER_H

#include <QJniObject>

#include "protocols/vpnprotocol.h"

using namespace amnezia;

class AndroidController : public QObject
{
    Q_OBJECT

public:
    explicit AndroidController();
    static AndroidController *instance();

    bool initialize();

    ErrorCode start(const QJsonObject &vpnConfig);
    void stop();
    void setNotificationText(const QString &title, const QString &message, int timerSec);
    void saveFile(const QString& fileName, const QString &data);
    void startQrReaderActivity();

signals:
    void connectionStateChanged(Vpn::ConnectionState state);
    void status(bool isVpnConnected);
    void serviceDisconnected();
    void serviceError();
    void vpnPermissionRejected();
    void vpnConnected();
    void vpnDisconnected();
    void statisticsUpdated(quint64 rxBytes, quint64 txBytes);
    void configImported();
    void importConfigFromOutside(QString &data);
    void serviceIsAlive(bool connected);

private:
    bool isWaitingInitStatus = true;

    void qtAndroidControllerInitialized();

    // JNI functions called by Android
    static void onStatus(JNIEnv *env, jobject thiz, jboolean isVpnConnected);
    static void onServiceDisconnected(JNIEnv *env, jobject thiz);
    static void onServiceError(JNIEnv *env, jobject thiz);
    static void onVpnPermissionRejected(JNIEnv *env, jobject thiz);
    static void onVpnConnected(JNIEnv *env, jobject thiz);
    static void onVpnDisconnected(JNIEnv *env, jobject thiz);
    static void onStatisticsUpdate(JNIEnv *env, jobject thiz, jlong rxBytes, jlong txBytes);
    static void onConfigImported(JNIEnv *env, jobject thiz);
    static bool decodeQrCode(JNIEnv *env, jobject thiz, jstring data);

    template <typename Ret, typename ...Args>
    static auto callActivityMethod(const char *methodName, const char *signature,
                                   const std::function<Ret()> &defValue, Args &&...args);
    template <typename ...Args>
    static void callActivityMethod(const char *methodName, const char *signature, Args &&...args);
};

#endif // ANDROID_CONTROLLER_H
