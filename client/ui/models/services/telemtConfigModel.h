#ifndef TELEMTCONFIGMODEL_H
#define TELEMTCONFIGMODEL_H

#include <QObject>
#include <QJsonObject>
#include <QRandomGenerator>

#include "core/models/protocols/telemtProtocolConfig.h"
#include "core/utils/containerEnum.h"

class TelemtConfigModel : public QObject {
Q_OBJECT

    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString secret READ secret WRITE setSecret NOTIFY changed)
    Q_PROPERTY(QString tag READ tag WRITE setTag NOTIFY changed)
    Q_PROPERTY(QString tgLink READ tgLink NOTIFY changed)
    Q_PROPERTY(QString tmeLink READ tmeLink NOTIFY changed)
    Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(QString publicHost READ publicHost WRITE setPublicHost NOTIFY changed)
    Q_PROPERTY(QString transportMode READ transportMode WRITE setTransportMode NOTIFY changed)
    Q_PROPERTY(QString tlsDomain READ tlsDomain WRITE setTlsDomain NOTIFY changed)
    Q_PROPERTY(QStringList additionalSecrets READ additionalSecrets WRITE setAdditionalSecrets NOTIFY changed)
    Q_PROPERTY(QString workersMode READ workersMode WRITE setWorkersMode NOTIFY changed)
    Q_PROPERTY(QString workers READ workers WRITE setWorkers NOTIFY changed)
    Q_PROPERTY(bool natEnabled READ natEnabled WRITE setNatEnabled NOTIFY changed)
    Q_PROPERTY(QString natInternalIp READ natInternalIp WRITE setNatInternalIp NOTIFY changed)
    Q_PROPERTY(QString natExternalIp READ natExternalIp WRITE setNatExternalIp NOTIFY changed)
    Q_PROPERTY(bool maskEnabled READ maskEnabled WRITE setMaskEnabled NOTIFY changed)
    Q_PROPERTY(bool useMiddleProxy READ useMiddleProxy WRITE setUseMiddleProxy NOTIFY changed)
    Q_PROPERTY(bool tlsEmulation READ tlsEmulation WRITE setTlsEmulation NOTIFY changed)
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY changed)

public:
    explicit TelemtConfigModel(QObject *parent = nullptr);

    QString port() const;

    QString secret() const;

    QString tag() const;

    QString tgLink() const;

    QString tmeLink() const;

    bool isEnabled() const;

    QString publicHost() const;

    QString transportMode() const;

    QString tlsDomain() const;

    QStringList additionalSecrets() const;

    void setAdditionalSecrets(const QStringList &secrets);

    QString workersMode() const;

    QString workers() const;

    bool natEnabled() const;

    QString natInternalIp() const;

    QString natExternalIp() const;

    bool maskEnabled() const;

    bool useMiddleProxy() const;

    bool tlsEmulation() const;

    QString userName() const;

public slots:

    void updateModel(amnezia::DockerContainer container, const amnezia::TelemtProtocolConfig &protocolConfig);

    void updateModel(const QJsonObject &config);

    QJsonObject getConfig();

    amnezia::TelemtProtocolConfig getProtocolConfig();

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

    Q_INVOKABLE QVariantList additionalSecretsList() const;

    Q_INVOKABLE QString generateQrCode(const QString &text);

    Q_INVOKABLE void setEnabled(bool enabled);

    Q_INVOKABLE void setMaskEnabled(bool enabled);

    Q_INVOKABLE void setUseMiddleProxy(bool enabled);

    Q_INVOKABLE void setTlsEmulation(bool enabled);

    Q_INVOKABLE void setUserName(const QString &name);

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

    Q_INVOKABLE bool isFakeTlsDomainTypingIncomplete(const QString &text) const;

    Q_INVOKABLE bool isFakeTlsDomainInputAllowed(const QString &text) const;

    Q_INVOKABLE QString sanitizeFakeTlsDomainFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizePublicHostFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizePortFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizeMtProxyTagFieldText(const QString &input) const;

    Q_INVOKABLE QString sanitizeOptionalIpv4FieldText(const QString &input) const;

    Q_INVOKABLE bool isValidOptionalIpv4(const QString &ip) const;

signals:
    void changed();

private:
    static void applyDefaults(amnezia::TelemtProtocolConfig &c);

    QString normalizeFakeTlsDomainInput(const QString &input) const;

    amnezia::DockerContainer m_container = amnezia::DockerContainer::None;
    QJsonObject m_fullConfig;
    amnezia::TelemtProtocolConfig m_protocolConfig;
};

#endif // TELEMTCONFIGMODEL_H
