#ifndef OPENVPNCONFIGMODEL_H
#define OPENVPNCONFIGMODEL_H

#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/openVpnProtocolConfig.h"

class OpenVpnConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString subnetAddress READ subnetAddress WRITE setSubnetAddress NOTIFY changed)
    Q_PROPERTY(QString transportProto READ transportProto WRITE setTransportProto NOTIFY changed)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(bool autoNegotiateEncryprion READ autoNegotiateEncryprion WRITE setAutoNegotiateEncryprion NOTIFY changed)
    Q_PROPERTY(QString hash READ hash WRITE setHash NOTIFY changed)
    Q_PROPERTY(QString cipher READ cipher WRITE setCipher NOTIFY changed)
    Q_PROPERTY(bool tlsAuth READ tlsAuth WRITE setTlsAuth NOTIFY changed)
    Q_PROPERTY(bool blockDns READ blockDns WRITE setBlockDns NOTIFY changed)
    Q_PROPERTY(QString additionalClientCommands READ additionalClientCommands WRITE setAdditionalClientCommands NOTIFY changed)
    Q_PROPERTY(QString additionalServerCommands READ additionalServerCommands WRITE setAdditionalServerCommands NOTIFY changed)
    Q_PROPERTY(bool isPortEditable READ isPortEditable NOTIFY changed)
    Q_PROPERTY(bool isTransportProtoEditable READ isTransportProtoEditable NOTIFY changed)
    Q_PROPERTY(bool hasRemoveButton READ hasRemoveButton NOTIFY changed)

public:
    explicit OpenVpnConfigModel(QObject *parent = nullptr);

    QString subnetAddress() const;
    void setSubnetAddress(const QString &value);

    QString transportProto() const;
    void setTransportProto(const QString &value);

    QString port() const;
    void setPort(const QString &value);

    bool autoNegotiateEncryprion() const;
    void setAutoNegotiateEncryprion(bool value);

    QString hash() const;
    void setHash(const QString &value);

    QString cipher() const;
    void setCipher(const QString &value);

    bool tlsAuth() const;
    void setTlsAuth(bool value);

    bool blockDns() const;
    void setBlockDns(bool value);

    QString additionalClientCommands() const;
    void setAdditionalClientCommands(const QString &value);

    QString additionalServerCommands() const;
    void setAdditionalServerCommands(const QString &value);

    bool isPortEditable() const;
    bool isTransportProtoEditable() const;
    bool hasRemoveButton() const;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::OpenVpnProtocolConfig &protocolConfig);
    amnezia::OpenVpnProtocolConfig getProtocolConfig();

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    amnezia::OpenVpnProtocolConfig m_protocolConfig;
    amnezia::OpenVpnProtocolConfig m_originalProtocolConfig;
    
    void applyDefaultsToServerConfig(amnezia::OpenVpnServerConfig& config);
    void applyDefaultsToClientConfig(amnezia::OpenVpnClientConfig& config);
};

#endif // OPENVPNCONFIGMODEL_H
