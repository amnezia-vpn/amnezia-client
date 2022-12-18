#ifndef ANDROID_CONTROLLER_H
#define ANDROID_CONTROLLER_H

//#include <QAndroidBinder>
//#include <QAndroidServiceConnection>
#include <QtCore/private/qandroidextras_p.h>

#include "ui/uilogic.h"
#include "ui/pages_logic/StartPageLogic.h"

#include "protocols/vpnprotocol.h"
using namespace amnezia;


class AndroidController : public QObject, public QAndroidServiceConnection
{
    Q_OBJECT

public:
    explicit AndroidController();
    static AndroidController* instance();

    virtual ~AndroidController() override = default;

    bool initialize(StartPageLogic *startPageLogic);

    ErrorCode start();
    void stop();
    void resumeStart();

    void checkStatus();
    void setNotificationText(const QString& title, const QString& message, int timerSec);
    void shareConfig(const QString& data, const QString& suggestedName);
    void setFallbackConnectedNotification();
    void getBackendLogs(std::function<void(const QString&)>&& callback);
    void cleanupBackendLogs();
    void importConfig(const QString& data);

    // from QAndroidServiceConnection
    void onServiceConnected(const QString& name, const QAndroidBinder& serviceBinder) override;
    void onServiceDisconnected(const QString& name) override;

    const QJsonObject &vpnConfig() const;
    void setVpnConfig(const QJsonObject &newVpnConfig);

signals:
    void connectionStateChanged(VpnProtocol::VpnConnectionState state);

    // This signal is emitted when the controller is initialized. Note that the
    // VPN tunnel can be already active. In this case, "connected" should be set
    // to true and the "connectionDate" should be set to the activation date if
    // known.
    // If "status" is set to false, the backend service is considered unavailable.
    void initialized(bool status, bool connected,
                     const QDateTime& connectionDate);

protected slots:

protected:


private:
    //Protocol m_protocol;
    QJsonObject m_vpnConfig;

    StartPageLogic *m_startPageLogic;

    bool m_serviceConnected = false;
    std::function<void(const QString&)> m_logCallback;

    QAndroidBinder m_serviceBinder;
    class VPNBinder : public QAndroidBinder {
     public:
      VPNBinder(AndroidController* controller) : m_controller(controller) {}

      bool onTransact(int code, const QAndroidParcel& data,
                      const QAndroidParcel& reply,
                      QAndroidBinder::CallType flags) override;

      QString readUTF8Parcel(QAndroidParcel data);

     private:
      AndroidController* m_controller = nullptr;
    };

    VPNBinder m_binder;

    static void startActivityForResult(JNIEnv* env, jobject /*thiz*/, jobject intent);
};

#endif // ANDROID_CONTROLLER_H
