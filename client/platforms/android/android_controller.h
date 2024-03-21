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

    // keep synchronized with org.amnezia.vpn.protocol.ProtocolState
    enum class ConnectionState
    {
        DISCONNECTED,
        CONNECTED,
        CONNECTING,
        DISCONNECTING,
        RECONNECTING,
        UNKNOWN
    };

    ErrorCode start(const QJsonObject &vpnConfig);
    void stop();
    void resetLastServer(int serverIndex);
    void setNotificationText(const QString &title, const QString &message, int timerSec);
    void saveFile(const QString &fileName, const QString &data);
    QString openFile(const QString &filter);
    bool isCameraPresent();
    void startQrReaderActivity();
    void setSaveLogs(bool enabled);
    void exportLogsFile(const QString &fileName);
    void clearLogs();
    void setScreenshotsEnabled(bool enabled);
    QJsonArray getAppList();

    static bool initLogging();
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);

signals:
    void connectionStateChanged(Vpn::ConnectionState state);
    void status(ConnectionState state);
    void serviceDisconnected();
    void serviceError();
    void vpnPermissionRejected();
    void vpnStateChanged(ConnectionState state);
    void statisticsUpdated(quint64 rxBytes, quint64 txBytes);
    void fileOpened(QString uri);
    void configImported(QString config);
    void importConfigFromOutside(QString config);
    void initConnectionState(Vpn::ConnectionState state);

private:
    bool isWaitingStatus = true;

    static jclass log;
    static jmethodID logDebug;
    static jmethodID logInfo;
    static jmethodID logWarning;
    static jmethodID logError;
    static jmethodID logFatal;

    void qtAndroidControllerInitialized();

    static Vpn::ConnectionState convertState(ConnectionState state);
    static QString textConnectionState(ConnectionState state);

    // JNI functions called by Android
    static void onStatus(JNIEnv *env, jobject thiz, jint stateCode);
    static void onServiceDisconnected(JNIEnv *env, jobject thiz);
    static void onServiceError(JNIEnv *env, jobject thiz);
    static void onVpnPermissionRejected(JNIEnv *env, jobject thiz);
    static void onVpnStateChanged(JNIEnv *env, jobject thiz, jint stateCode);
    static void onStatisticsUpdate(JNIEnv *env, jobject thiz, jlong rxBytes, jlong txBytes);
    static void onConfigImported(JNIEnv *env, jobject thiz, jstring data);
    static void onFileOpened(JNIEnv *env, jobject thiz, jstring uri);
    static bool decodeQrCode(JNIEnv *env, jobject thiz, jstring data);

    template <typename Ret, typename ...Args>
    static auto callActivityMethod(const char *methodName, const char *signature, Args &&...args);
    template <typename ...Args>
    static void callActivityMethod(const char *methodName, const char *signature, Args &&...args);
};

#endif // ANDROID_CONTROLLER_H
