#ifndef WHATSAPPPROXYCONFIGMODEL_H
#define WHATSAPPPROXYCONFIGMODEL_H

#include "core/models/protocols/whatsAppProxyProtocolConfig.h"
#include "core/utils/containerEnum.h"
#include <QJsonObject>
#include <QObject>

class WhatsAppProxyConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString port READ getPort WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString mediaPort READ getMediaPort WRITE setMediaPort NOTIFY changed)
    Q_PROPERTY(QString publicHost READ getPublicHost WRITE setPublicHost NOTIFY changed)
    Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled NOTIFY changed)

public:
    explicit WhatsAppProxyConfigModel(QObject *parent = nullptr);

    bool isEnabled() const;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::WhatsAppProxyProtocolConfig &protocolConfig);
    void updateModel(const QJsonObject &config);

    QJsonObject getConfig();
    amnezia::WhatsAppProxyProtocolConfig getProtocolConfig();

    Q_INVOKABLE void setPort(const QString &port);
    Q_INVOKABLE void setMediaPort(const QString &port);
    Q_INVOKABLE void setPublicHost(const QString &host);
    Q_INVOKABLE void setEnabled(bool enabled);

    Q_INVOKABLE QString getPort() const;
    Q_INVOKABLE QString getMediaPort() const;
    Q_INVOKABLE QString getPublicHost() const;

    Q_INVOKABLE QString defaultPort() const;
    Q_INVOKABLE QString defaultMediaPort() const;

    Q_INVOKABLE bool isValidPort(const QString &port) const;
    Q_INVOKABLE bool isValidPublicHost(const QString &host) const;
    Q_INVOKABLE QString sanitizePortFieldText(const QString &input) const;

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    QJsonObject m_fullConfig;
    amnezia::WhatsAppProxyProtocolConfig m_protocolConfig;
};

#endif // WHATSAPPPROXYCONFIGMODEL_H
