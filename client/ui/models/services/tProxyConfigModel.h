#ifndef TPROXYCONFIGMODEL_H
#define TPROXYCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include "core/utils/containerEnum.h"
#include "core/models/protocols/tProxyProtocolConfig.h"

class TProxyConfigModel : public QAbstractListModel {
Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        HttpPortRole,
        SecretRole,
        HostnameRole,
        AcmeEmailRole,
        CarrierModeRole,
        WorkersRole,
        TgLinkRole,
        TmeLinkRole,
        IsEnabledRole
    };

    explicit TProxyConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

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

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    QJsonObject m_fullConfig;
    amnezia::TProxyProtocolConfig m_protocolConfig;
};

#endif // TPROXYCONFIGMODEL_H
