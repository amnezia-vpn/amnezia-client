#ifndef TPROXYCONFIGMODEL_H
#define TPROXYCONFIGMODEL_H

#include <QObject>
#include <QJsonObject>
#include "core/utils/containerEnum.h"
#include "core/models/protocols/tProxyProtocolConfig.h"

class TProxyConfigModel : public QObject {
Q_OBJECT

    Q_PROPERTY(QString port READ getPort WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString httpPort READ getHttpPort WRITE setHttpPort NOTIFY changed)
    Q_PROPERTY(QString secret READ getSecret WRITE setSecret NOTIFY changed)
    Q_PROPERTY(QString hostname READ getHostname WRITE setHostname NOTIFY changed)
    Q_PROPERTY(QString acmeEmail READ getAcmeEmail WRITE setAcmeEmail NOTIFY changed)
    Q_PROPERTY(QString carrierMode READ getCarrierMode WRITE setCarrierMode NOTIFY changed)
    Q_PROPERTY(QString workers READ getWorkers WRITE setWorkers NOTIFY changed)
    Q_PROPERTY(QString tgLink READ tgLink NOTIFY changed)
    Q_PROPERTY(QString tmeLink READ tmeLink NOTIFY changed)
    Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled NOTIFY changed)

public:
    explicit TProxyConfigModel(QObject *parent = nullptr);

    QString tgLink() const;
    QString tmeLink() const;
    bool isEnabled() const;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::TProxyProtocolConfig &protocolConfig);
    void updateModel(const QJsonObject &config);

    QJsonObject getConfig();
    amnezia::TProxyProtocolConfig getProtocolConfig();

    Q_INVOKABLE void generateSecret();
    Q_INVOKABLE void setSecret(const QString &secret);
    Q_INVOKABLE bool validateAndSetSecret(const QString &rawSecret);
    Q_INVOKABLE void setPort(const QString &port);
    Q_INVOKABLE void setHttpPort(const QString &port);
    Q_INVOKABLE void setHostname(const QString &hostname);
    Q_INVOKABLE void setAcmeEmail(const QString &email);
    Q_INVOKABLE void setCarrierMode(const QString &mode);
    Q_INVOKABLE void setWorkers(const QString &workers);
    Q_INVOKABLE void setEnabled(bool enabled);

    Q_INVOKABLE QString getHostname() const;
    Q_INVOKABLE QString getSecret() const;
    Q_INVOKABLE QString getAcmeEmail() const;
    Q_INVOKABLE QString getCarrierMode() const;
    Q_INVOKABLE QString getWorkers() const;
    Q_INVOKABLE QString getPort() const;
    Q_INVOKABLE QString getHttpPort() const;

    Q_INVOKABLE QString defaultPort() const;
    Q_INVOKABLE QString defaultHttpPort() const;
    Q_INVOKABLE QString defaultWorkers() const;
    Q_INVOKABLE int maxWorkers() const;
    Q_INVOKABLE QString carrierModeHttps() const;

    Q_INVOKABLE bool isValidHostname(const QString &host) const;
    Q_INVOKABLE bool isHostnameTypingIncomplete(const QString &text) const;
    Q_INVOKABLE QString sanitizeHostnameFieldText(const QString &input) const;

    Q_INVOKABLE bool isValidAcmeEmail(const QString &email) const;
    Q_INVOKABLE bool isAcmeEmailTypingIncomplete(const QString &text) const;
    Q_INVOKABLE QString sanitizeAcmeEmailFieldText(const QString &input) const;

    Q_INVOKABLE bool isValidCarrierMode(const QString &mode) const;
    Q_INVOKABLE QString carrierModeLabel(const QString &mode) const;

    Q_INVOKABLE QString sanitizeWorkersFieldText(const QString &input) const;
    Q_INVOKABLE QString sanitizePortFieldText(const QString &input) const;

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    QJsonObject m_fullConfig;
    amnezia::TProxyProtocolConfig m_protocolConfig;
};

#endif // TPROXYCONFIGMODEL_H
