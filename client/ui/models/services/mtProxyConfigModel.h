#ifndef MTPROXYCONFIGMODEL_H
#define MTPROXYCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/mtProxyProtocolConfig.h"

class MtProxyConfigModel : public QAbstractListModel {
Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        SecretRole,
        TagRole,
        TgLinkRole,
        TmeLinkRole,
        IsEnabledRole,
        PublicHostRole,
        TransportModeRole,
        TlsDomainRole,
        AdditionalSecretsRole,
        WorkersModeRole,
        WorkersRole,
        NatEnabledRole,
        NatInternalIpRole,
        NatExternalIpRole
    };

    explicit MtProxyConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:

    void updateModel(amnezia::DockerContainer container, const amnezia::MtProxyProtocolConfig &protocolConfig);

    void updateModel(const QJsonObject &config);

    QJsonObject getConfig();

    amnezia::MtProxyProtocolConfig getProtocolConfig();

    Q_INVOKABLE void generateSecret();

    Q_INVOKABLE void setSecret(const QString &secret);

    Q_INVOKABLE bool validateAndSetSecret(const QString &rawSecret);

    Q_INVOKABLE void setPort(const QString &port);

    Q_INVOKABLE void setTag(const QString &tag);

    Q_INVOKABLE void setPublicHost(const QString &host);

    Q_INVOKABLE void setTransportMode(const QString &mode);

    Q_INVOKABLE QString getTransportMode() const;

    Q_INVOKABLE QString getTlsDomain() const;

    Q_INVOKABLE QString getPublicHost() const;

    Q_INVOKABLE void setTlsDomain(const QString &domain);

    Q_INVOKABLE void setWorkersMode(const QString &mode);

    Q_INVOKABLE void setWorkers(const QString &workers);

    Q_INVOKABLE void setNatEnabled(bool enabled);

    Q_INVOKABLE void setNatInternalIp(const QString &ip);

    Q_INVOKABLE void setNatExternalIp(const QString &ip);

    Q_INVOKABLE void addAdditionalSecret();

    Q_INVOKABLE void removeAdditionalSecret(int idx);
    /// Current `mtproxy_additional_secrets` list from in-memory config (for QML snapshot vs. unsaved adds).
    Q_INVOKABLE QVariantList additionalSecretsList() const;

    Q_INVOKABLE QString generateQrCode(const QString &text);

    Q_INVOKABLE void setEnabled(bool enabled);

    Q_INVOKABLE QString defaultTlsDomain() const;

    Q_INVOKABLE QString defaultPort() const;

    Q_INVOKABLE QString defaultWorkers() const;

    Q_INVOKABLE int maxWorkers() const;

    Q_INVOKABLE QString transportModeStandard() const;

    Q_INVOKABLE QString transportModeFakeTLS() const;

    Q_INVOKABLE QString workersModeAuto() const;

    Q_INVOKABLE QString workersModeManual() const;

    Q_INVOKABLE bool isValidPublicHost(const QString &host) const;

    Q_INVOKABLE bool isPublicHostInputAllowed(const QString &text) const;

    Q_INVOKABLE bool isPublicHostTypingIncomplete(const QString &text) const;

    Q_INVOKABLE bool isValidMtProxyTag(const QString &tag) const;

    Q_INVOKABLE bool isMtProxyTagTypingIncomplete(const QString &text) const;

    Q_INVOKABLE int mtProxyBotTagHexLength() const;

    Q_INVOKABLE bool isValidFakeTlsDomain(const QString &domain) const;

    Q_INVOKABLE QString normalizeFakeTlsDomainInput(const QString &input) const;

    Q_INVOKABLE QString sanitizeFakeTlsDomainFieldText(const QString &input) const;

    Q_INVOKABLE bool isFakeTlsDomainInputAllowed(const QString &text) const;

    Q_INVOKABLE QString clipboardText() const;

    Q_INVOKABLE QString sanitizePublicHostFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizePortFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizeMtProxyTagFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizeWorkersFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizeOptionalIpv4FieldText(const QString &input) const;

    Q_INVOKABLE bool isFakeTlsDomainTypingIncomplete(const QString &text) const;

    Q_INVOKABLE bool isValidOptionalIpv4(const QString &ip) const;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    QJsonObject m_fullConfig;
    amnezia::MtProxyProtocolConfig m_protocolConfig;
};

#endif // MTPROXYCONFIGMODEL_H
