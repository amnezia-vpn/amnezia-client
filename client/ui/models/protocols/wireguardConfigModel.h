#ifndef WIREGUARDCONFIGMODEL_H
#define WIREGUARDCONFIGMODEL_H

#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"

class WireGuardConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString subnetAddress READ subnetAddress WRITE setSubnetAddress NOTIFY changed)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString clientMtu READ clientMtu WRITE setClientMtu NOTIFY changed)

public:
    explicit WireGuardConfigModel(QObject *parent = nullptr);

    QString subnetAddress() const;
    void setSubnetAddress(const QString &value);

    QString port() const;
    void setPort(const QString &value);

    QString clientMtu() const;
    void setClientMtu(const QString &value);

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::WireGuardProtocolConfig &protocolConfig);
    amnezia::WireGuardProtocolConfig getProtocolConfig();

    bool isServerSettingsEqual();

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    amnezia::WireGuardProtocolConfig m_protocolConfig;
    amnezia::WireGuardProtocolConfig m_originalProtocolConfig;
    
    void applyDefaultsToServerConfig(amnezia::WireGuardServerConfig& config);
    void applyDefaultsToClientConfig(amnezia::WireGuardClientConfig& config);
};

#endif // WIREGUARDCONFIGMODEL_H
