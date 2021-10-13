#ifndef IPSEC_PROTOCOL_H
#define IPSEC_PROTOCOL_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/ipcclient.h"

#ifdef Q_OS_WIN
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

#endif

class Ikev2Protocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit Ikev2Protocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~Ikev2Protocol() override;

    ErrorCode start() override;
    void stop() override;

    static QString tunnelName() { return "AmneziaVPN IKEv2"; }

private:
    void readIkev2Configuration(const QJsonObject &configuration);

#ifdef Q_OS_WIN
    //certificates variables

#endif
private:
    QJsonObject m_config;

#ifdef Q_OS_WIN
    //RAS functions and parametrs
    HRASCONN        hRasConn{nullptr};
//    bool create_new_vpn(const QString & vpn_name,
//                        const QString & serv_addr);
    bool delete_vpn_connection(const QString &vpn_name);

    bool connect_to_vpn(const QString & vpn_name);
    bool disconnect_vpn();
    static void WINAPI RasDialFuncCallback(UINT unMsg,
                                   RASCONNSTATE rasconnstate,
                                   DWORD dwError );


    std::unique_ptr<std::thread> _thr{nullptr};
    void _ikev2_states();
    std::atomic_bool _stoped{false};

    std::unique_ptr<std::thread>_th_conn_state{nullptr};
    void conn_state();
signals:
    //void Ikev2_connected();
    //void Ikev2_disconnected();
    //void Ikev2_state(int);
#endif

};

#ifdef Q_OS_WIN
DWORD CALLBACK rasCallback(UINT msg, RASCONNSTATE rascs, DWORD err);
#endif

#endif // IPSEC_PROTOCOL_H
