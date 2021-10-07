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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "rasapi32.lib")

#define RASBUFFER       0x1000
#define RASMAXENUM      0x100
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


private:
    QJsonObject m_config;

#ifdef Q_OS_WIN
    HRASCONN g_h;
    int      g_done = 0;
#endif
};

#ifdef Q_OS_WIN
DWORD CALLBACK rasCallback(UINT msg, RASCONNSTATE rascs, DWORD err);
#endif

#endif // IPSEC_PROTOCOL_H
