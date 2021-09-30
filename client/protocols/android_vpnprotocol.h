#ifndef ANDROID_VPNPROTOCOL_H
#define ANDROID_VPNPROTOCOL_H

#include <QAndroidBinder>
#include <QAndroidServiceConnection>

#include "vpnprotocol.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;



class AndroidVpnProtocol : public VpnProtocol,
                           public QAndroidServiceConnection
{
    Q_OBJECT

public:
    explicit AndroidVpnProtocol(Protocol protocol, const QJsonObject& configuration, QObject* parent = nullptr);
    static AndroidVpnProtocol* instance();

    virtual ~AndroidVpnProtocol() override = default;

    void initialize();

    virtual ErrorCode start() override;
    virtual void stop() override;

    void resume_start();

    void checkStatus();

    void setNotificationText(const QString& title, const QString& message,
                             int timerSec);
    void setFallbackConnectedNotification();

    void getBackendLogs(std::function<void(const QString&)>&& callback);

    void cleanupBackendLogs();

    // from QAndroidServiceConnection
    void onServiceConnected(const QString& name,
                            const QAndroidBinder& serviceBinder) override;
    void onServiceDisconnected(const QString& name) override;

signals:


protected slots:

protected:


private:
    Protocol m_protocol;

    bool m_serviceConnected = false;
    std::function<void(const QString&)> m_logCallback;

    QAndroidBinder m_serviceBinder;
    class VPNBinder : public QAndroidBinder {
     public:
      VPNBinder(AndroidVpnProtocol* controller) : m_controller(controller) {}

      bool onTransact(int code, const QAndroidParcel& data,
                      const QAndroidParcel& reply,
                      QAndroidBinder::CallType flags) override;

      QString readUTF8Parcel(QAndroidParcel data);

     private:
      AndroidVpnProtocol* m_controller = nullptr;
    };

    VPNBinder m_binder;

    static void startActivityForResult(JNIEnv* env, jobject /*thiz*/,
                                       jobject intent);
};

#endif // ANDROID_VPNPROTOCOL_H
