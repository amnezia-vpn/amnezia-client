#ifndef SFTPCONFIGMODEL_H
#define SFTPCONFIGMODEL_H

#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/sftpProtocolConfig.h"

class SftpConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY changed)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY changed)

public:
    explicit SftpConfigModel(QObject *parent = nullptr);

    QString port() const;
    void setPort(const QString &value);

    QString username() const;
    void setUsername(const QString &value);

    QString password() const;
    void setPassword(const QString &value);

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::SftpProtocolConfig &protocolConfig);
    amnezia::SftpProtocolConfig getProtocolConfig();

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    amnezia::SftpProtocolConfig m_protocolConfig;
    
    void applyDefaults(amnezia::SftpProtocolConfig& config);
};

#endif // SFTPCONFIGMODEL_H
