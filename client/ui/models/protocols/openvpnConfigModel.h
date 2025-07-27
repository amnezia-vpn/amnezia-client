#ifndef OPENVPNCONFIGMODEL_H
#define OPENVPNCONFIGMODEL_H

#include <QObject>
#include <QJsonObject>

#include "containers/containers_defs.h"

class OpenVpnConfigModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString subnetAddress READ subnetAddress WRITE setSubnetAddress NOTIFY subnetAddressChanged)
    Q_PROPERTY(QString transportProto READ transportProto WRITE setTransportProto NOTIFY transportProtoChanged)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool autoNegotiateEncryption READ autoNegotiateEncryption WRITE setAutoNegotiateEncryption NOTIFY autoNegotiateEncryptionChanged)
    Q_PROPERTY(QString hash READ hash WRITE setHash NOTIFY hashChanged)
    Q_PROPERTY(QString cipher READ cipher WRITE setCipher NOTIFY cipherChanged)
    Q_PROPERTY(bool tlsAuth READ tlsAuth WRITE setTlsAuth NOTIFY tlsAuthChanged)
    Q_PROPERTY(bool blockDns READ blockDns WRITE setBlockDns NOTIFY blockDnsChanged)
    Q_PROPERTY(QString additionalClientCommands READ additionalClientCommands WRITE setAdditionalClientCommands NOTIFY additionalClientCommandsChanged)
    Q_PROPERTY(QString additionalServerCommands READ additionalServerCommands WRITE setAdditionalServerCommands NOTIFY additionalServerCommandsChanged)
    Q_PROPERTY(bool isPortEditable READ isPortEditable NOTIFY isPortEditableChanged)
    Q_PROPERTY(bool isTransportProtoEditable READ isTransportProtoEditable NOTIFY isTransportProtoEditableChanged)
    Q_PROPERTY(bool hasRemoveButton READ hasRemoveButton NOTIFY hasRemoveButtonChanged)

public:
    explicit OpenVpnConfigModel(QObject *parent = nullptr);
    ~OpenVpnConfigModel() override = default;

    OpenVpnConfigModel(const OpenVpnConfigModel &) = delete;
    OpenVpnConfigModel &operator=(const OpenVpnConfigModel &) = delete;
    OpenVpnConfigModel(OpenVpnConfigModel &&) = delete;
    OpenVpnConfigModel &operator=(OpenVpnConfigModel &&) = delete;

    QString subnetAddress() const;
    void setSubnetAddress(const QString &subnetAddress);

    QString transportProto() const;
    void setTransportProto(const QString &transportProto);

    QString port() const;
    void setPort(const QString &port);

    bool autoNegotiateEncryption() const;
    void setAutoNegotiateEncryption(bool enabled);

    QString hash() const;
    void setHash(const QString &hash);

    QString cipher() const;
    void setCipher(const QString &cipher);

    bool tlsAuth() const;
    void setTlsAuth(bool enabled);

    bool blockDns() const;
    void setBlockDns(bool enabled);

    QString additionalClientCommands() const;
    void setAdditionalClientCommands(const QString &commands);
    
    QString additionalServerCommands() const;
    void setAdditionalServerCommands(const QString &commands);

    bool isPortEditable() const;
    bool isTransportProtoEditable() const;
    bool hasRemoveButton() const;

    Q_INVOKABLE QJsonObject getConfig();

signals:
    void subnetAddressChanged(const QString &);
    void transportProtoChanged(const QString &);
    void portChanged(const QString &);
    void autoNegotiateEncryptionChanged(bool);
    void hashChanged(const QString &);
    void cipherChanged(const QString &);
    void tlsAuthChanged(bool);
    void blockDnsChanged(bool);
    void additionalClientCommandsChanged(const QString &);
    void additionalServerCommandsChanged(const QString &);
    void isPortEditableChanged(bool);
    void isTransportProtoEditableChanged(bool);
    void hasRemoveButtonChanged(bool);

public slots:
    void updateModel(const QJsonObject &config);

private:
    DockerContainer m_container;
    QJsonObject m_protocolConfig;
    QJsonObject m_fullConfig;
};

#endif // OPENVPNCONFIGMODEL_H
