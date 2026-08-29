#ifndef DNSTTCONFIGMODEL_H
#define DNSTTCONFIGMODEL_H

#include <QObject>
#include <QString>
#include "core/models/protocols/dnsttProtocolConfig.h"

class DnsttConfigModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    Q_PROPERTY(QString resolvers READ resolvers WRITE setResolvers NOTIFY resolversChanged)
    Q_PROPERTY(QString bootstrapIp READ bootstrapIp WRITE setBootstrapIp NOTIFY bootstrapIpChanged)
    Q_PROPERTY(QString publicKey READ publicKey WRITE setPublicKey NOTIFY publicKeyChanged)
    Q_PROPERTY(int calculatedMtu READ calculatedMtu NOTIFY calculatedMtuChanged)
    Q_PROPERTY(bool isMtuValid READ isMtuValid NOTIFY isMtuValidChanged)
    Q_PROPERTY(bool isPublicKeyValid READ isPublicKeyValid NOTIFY isPublicKeyValidChanged)
    Q_PROPERTY(bool needsBootstrap READ needsBootstrap NOTIFY needsBootstrapChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)

public:
    explicit DnsttConfigModel(QObject *parent = nullptr);

    QString domain() const;
    void setDomain(const QString &domain);

    QString resolvers() const;
    void setResolvers(const QString &resolvers);

    QString bootstrapIp() const;
    void setBootstrapIp(const QString &bootstrapIp);

    QString publicKey() const;
    void setPublicKey(const QString &publicKey);

    int calculatedMtu() const;
    bool isMtuValid() const;
    bool isPublicKeyValid() const;
    bool needsBootstrap() const;
    bool isValid() const;

    Q_INVOKABLE QString getValidationError() const;
    Q_INVOKABLE QString generateUri() const;

signals:
    void domainChanged();
    void resolversChanged();
    void bootstrapIpChanged();
    void publicKeyChanged();
    void calculatedMtuChanged();
    void isMtuValidChanged();
    void isPublicKeyValidChanged();
    void needsBootstrapChanged();
    void isValidChanged();

private:
    amnezia::DnsttProtocolConfig m_config;
};

#endif // DNSTTCONFIGMODEL_H
