#ifndef IKEV2_VPN_PROTOCOL_WINDOWS_H
#define IKEV2_VPN_PROTOCOL_WINDOWS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/ipcclient.h"

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>


#include <stdio.h>
#include <windows.h>
#include <Ras.h>
#include <raserror.h>
#include <shlwapi.h>

#include <wincrypt.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "rasapi32.lib")
#pragma comment(lib, "Crypt32.lib")

class Ikev2Protocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit Ikev2Protocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~Ikev2Protocol() override;

    ErrorCode start() override;
    void stop() override;

    static QString tunnelName() { return "AmneziaVPN IKEv2"; }

public:
    void newConnectionStateEventReceived(UINT unMsg,
                                         RASCONNSTATE rasconnstate,
                                         DWORD dwError);

private:
    void readIkev2Configuration(const QJsonObject &configuration);

private:
    QJsonObject m_config;

    //RAS functions and parameters
    HRASCONN        hRasConn{nullptr};
    bool create_new_vpn(const QString & vpn_name,
                        const QString & serv_addr);
    bool delete_vpn_connection(const QString &vpn_name);

    bool connect_to_vpn(const QString & vpn_name);
    bool disconnect_vpn();
};

DWORD CALLBACK rasCallback(UINT msg, RASCONNSTATE rascs, DWORD err);

#endif // IKEV2_VPN_PROTOCOL_WINDOWS_H
