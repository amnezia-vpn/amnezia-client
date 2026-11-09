#ifndef SOCKS5PROXYCONFIGMODEL_H
#define SOCKS5PROXYCONFIGMODEL_H

#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"

class Socks5ProxyConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY changed)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY changed)

public:
    explicit Socks5ProxyConfigModel(QObject *parent = nullptr);

    QString port() const;
    void setPort(const QString &value);

    QString username() const;
    void setUsername(const QString &value);

    QString password() const;
    void setPassword(const QString &value);

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::Socks5ProxyProtocolConfig &protocolConfig);
    amnezia::Socks5ProxyProtocolConfig getProtocolConfig();

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    amnezia::Socks5ProxyProtocolConfig m_protocolConfig;
};

#endif // SOCKS5PROXYCONFIGMODEL_H
