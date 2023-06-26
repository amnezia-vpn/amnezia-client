/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ANDROID_CONTROLLER_H
#define ANDROID_CONTROLLER_H

#include <QJniEnvironment>
#include <QJniObject>

#include "ui/pages_logic/StartPageLogic.h"

#include "protocols/vpnprotocol.h"

using namespace amnezia;


class AndroidController : public QObject
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

    const QJsonObject &vpnConfig() const;
    void setVpnConfig(const QJsonObject &newVpnConfig);

    void startQrReaderActivity();

signals:
    void connectionStateChanged(VpnProtocol::VpnConnectionState state);

    // This signal is emitted when the controller is initialized. Note that the
    // VPN tunnel can be already active. In this case, "connected" should be set
    // to true and the "connectionDate" should be set to the activation date if
    // known.
    // If "status" is set to false, the backend service is considered unavailable.
    void initialized(bool status, bool connected, const QDateTime& connectionDate);

    void statusUpdated(QString totalRx, QString totalTx, QString endpoint, QString deviceIPv4);
    void scheduleStatusCheckSignal();

protected slots:
    void scheduleStatusCheckSlot();

private:
    bool m_init = false;

    QJsonObject m_vpnConfig;

    StartPageLogic *m_startPageLogic;

    bool m_serviceConnected = false;
    std::function<void(const QString&)> m_logCallback;

    static void startActivityForResult(JNIEnv* env, jobject /*thiz*/, jobject intent);

    bool isConnected = false;

    void scheduleStatusCheck();
};

#endif // ANDROID_CONTROLLER_H
