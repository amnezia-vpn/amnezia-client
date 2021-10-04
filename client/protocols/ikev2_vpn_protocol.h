#ifndef IPSEC_PROTOCOL_H
#define IPSEC_PROTOCOL_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/ipcclient.h"

class Ikev2Protocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit Ikev2Protocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~Ikev2Protocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    void readIkev2Configuration(const QJsonObject &configuration);


private:
    QJsonObject m_config;
};

#endif // IPSEC_PROTOCOL_H
