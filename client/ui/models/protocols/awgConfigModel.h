#ifndef AWGCONFIGMODEL_H
#define AWGCONFIGMODEL_H

#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/awgProtocolConfig.h"

class AwgConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString subnetAddress READ subnetAddress WRITE setSubnetAddress NOTIFY changed)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString clientMtu READ clientMtu WRITE setClientMtu NOTIFY changed)
    Q_PROPERTY(QString clientJunkPacketCount READ clientJunkPacketCount WRITE setClientJunkPacketCount NOTIFY changed)
    Q_PROPERTY(QString clientJunkPacketMinSize READ clientJunkPacketMinSize WRITE setClientJunkPacketMinSize NOTIFY changed)
    Q_PROPERTY(QString clientJunkPacketMaxSize READ clientJunkPacketMaxSize WRITE setClientJunkPacketMaxSize NOTIFY changed)
    Q_PROPERTY(QString clientSpecialJunk1 READ clientSpecialJunk1 WRITE setClientSpecialJunk1 NOTIFY changed)
    Q_PROPERTY(QString clientSpecialJunk2 READ clientSpecialJunk2 WRITE setClientSpecialJunk2 NOTIFY changed)
    Q_PROPERTY(QString clientSpecialJunk3 READ clientSpecialJunk3 WRITE setClientSpecialJunk3 NOTIFY changed)
    Q_PROPERTY(QString clientSpecialJunk4 READ clientSpecialJunk4 WRITE setClientSpecialJunk4 NOTIFY changed)
    Q_PROPERTY(QString clientSpecialJunk5 READ clientSpecialJunk5 WRITE setClientSpecialJunk5 NOTIFY changed)
    Q_PROPERTY(QString clientContentPaddingAddition READ clientContentPaddingAddition WRITE setClientContentPaddingAddition NOTIFY changed)
    Q_PROPERTY(QString clientRekeyAfterTime READ clientRekeyAfterTime WRITE setClientRekeyAfterTime NOTIFY changed)
    Q_PROPERTY(QString clientRekeyTimeout READ clientRekeyTimeout WRITE setClientRekeyTimeout NOTIFY changed)
    Q_PROPERTY(QString clientRejectAfterTime READ clientRejectAfterTime WRITE setClientRejectAfterTime NOTIFY changed)
    Q_PROPERTY(QString clientKeepaliveTimeout READ clientKeepaliveTimeout WRITE setClientKeepaliveTimeout NOTIFY changed)
    Q_PROPERTY(QString clientMaxHandshakeAttempts READ clientMaxHandshakeAttempts WRITE setClientMaxHandshakeAttempts NOTIFY changed)
    Q_PROPERTY(bool clientHeaderProtectionEnabled READ clientHeaderProtectionEnabled NOTIFY changed)
    Q_PROPERTY(bool clientRandomTrailers READ clientRandomTrailers WRITE setClientRandomTrailers NOTIFY changed)
    Q_PROPERTY(bool clientDisableCookies READ clientDisableCookies WRITE setClientDisableCookies NOTIFY changed)
    Q_PROPERTY(QString serverJunkPacketCount READ serverJunkPacketCount WRITE setServerJunkPacketCount NOTIFY changed)
    Q_PROPERTY(QString serverJunkPacketMinSize READ serverJunkPacketMinSize WRITE setServerJunkPacketMinSize NOTIFY changed)
    Q_PROPERTY(QString serverJunkPacketMaxSize READ serverJunkPacketMaxSize WRITE setServerJunkPacketMaxSize NOTIFY changed)
    Q_PROPERTY(QString serverInitPacketJunkSize READ serverInitPacketJunkSize WRITE setServerInitPacketJunkSize NOTIFY changed)
    Q_PROPERTY(QString serverResponsePacketJunkSize READ serverResponsePacketJunkSize WRITE setServerResponsePacketJunkSize NOTIFY changed)
    Q_PROPERTY(QString serverCookieReplyPacketJunkSize READ serverCookieReplyPacketJunkSize WRITE setServerCookieReplyPacketJunkSize NOTIFY changed)
    Q_PROPERTY(QString serverTransportPacketJunkSize READ serverTransportPacketJunkSize WRITE setServerTransportPacketJunkSize NOTIFY changed)
    Q_PROPERTY(QString serverInitPacketMagicHeader READ serverInitPacketMagicHeader WRITE setServerInitPacketMagicHeader NOTIFY changed)
    Q_PROPERTY(QString serverResponsePacketMagicHeader READ serverResponsePacketMagicHeader WRITE setServerResponsePacketMagicHeader NOTIFY changed)
    Q_PROPERTY(QString serverUnderloadPacketMagicHeader READ serverUnderloadPacketMagicHeader WRITE setServerUnderloadPacketMagicHeader NOTIFY changed)
    Q_PROPERTY(QString serverTransportPacketMagicHeader READ serverTransportPacketMagicHeader WRITE setServerTransportPacketMagicHeader NOTIFY changed)
    Q_PROPERTY(QString serverSpecialJunk1 READ serverSpecialJunk1 WRITE setServerSpecialJunk1 NOTIFY changed)
    Q_PROPERTY(QString serverSpecialJunk2 READ serverSpecialJunk2 WRITE setServerSpecialJunk2 NOTIFY changed)
    Q_PROPERTY(QString serverSpecialJunk3 READ serverSpecialJunk3 WRITE setServerSpecialJunk3 NOTIFY changed)
    Q_PROPERTY(QString serverSpecialJunk4 READ serverSpecialJunk4 WRITE setServerSpecialJunk4 NOTIFY changed)
    Q_PROPERTY(QString serverSpecialJunk5 READ serverSpecialJunk5 WRITE setServerSpecialJunk5 NOTIFY changed)
    Q_PROPERTY(QString serverContentPaddingAddition READ serverContentPaddingAddition WRITE setServerContentPaddingAddition NOTIFY changed)
    Q_PROPERTY(QString serverRekeyAfterTime READ serverRekeyAfterTime WRITE setServerRekeyAfterTime NOTIFY changed)
    Q_PROPERTY(QString serverRekeyTimeout READ serverRekeyTimeout WRITE setServerRekeyTimeout NOTIFY changed)
    Q_PROPERTY(QString serverRejectAfterTime READ serverRejectAfterTime WRITE setServerRejectAfterTime NOTIFY changed)
    Q_PROPERTY(QString serverKeepaliveTimeout READ serverKeepaliveTimeout WRITE setServerKeepaliveTimeout NOTIFY changed)
    Q_PROPERTY(QString serverMaxHandshakeAttempts READ serverMaxHandshakeAttempts WRITE setServerMaxHandshakeAttempts NOTIFY changed)
    Q_PROPERTY(bool serverHeaderProtectionEnabled READ serverHeaderProtectionEnabled WRITE setServerHeaderProtectionEnabled NOTIFY changed)
    Q_PROPERTY(bool serverRandomTrailers READ serverRandomTrailers WRITE setServerRandomTrailers NOTIFY changed)
    Q_PROPERTY(bool serverDisableCookies READ serverDisableCookies WRITE setServerDisableCookies NOTIFY changed)
    Q_PROPERTY(bool isAwg2 READ isAwg2 NOTIFY changed)
    Q_PROPERTY(bool isAwg3 READ isAwg3 NOTIFY changed)

public:
    explicit AwgConfigModel(QObject *parent = nullptr);

    QString subnetAddress() const;
    void setSubnetAddress(const QString &value);

    QString port() const;
    void setPort(const QString &value);

    QString clientMtu() const;
    void setClientMtu(const QString &value);

    QString clientJunkPacketCount() const;
    void setClientJunkPacketCount(const QString &value);

    QString clientJunkPacketMinSize() const;
    void setClientJunkPacketMinSize(const QString &value);

    QString clientJunkPacketMaxSize() const;
    void setClientJunkPacketMaxSize(const QString &value);

    QString clientSpecialJunk1() const;
    void setClientSpecialJunk1(const QString &value);

    QString clientSpecialJunk2() const;
    void setClientSpecialJunk2(const QString &value);

    QString clientSpecialJunk3() const;
    void setClientSpecialJunk3(const QString &value);

    QString clientSpecialJunk4() const;
    void setClientSpecialJunk4(const QString &value);

    QString clientSpecialJunk5() const;
    void setClientSpecialJunk5(const QString &value);

    QString clientContentPaddingAddition() const;
    void setClientContentPaddingAddition(const QString &value);

    QString clientRekeyAfterTime() const;
    void setClientRekeyAfterTime(const QString &value);

    QString clientRekeyTimeout() const;
    void setClientRekeyTimeout(const QString &value);

    QString clientRejectAfterTime() const;
    void setClientRejectAfterTime(const QString &value);

    QString clientKeepaliveTimeout() const;
    void setClientKeepaliveTimeout(const QString &value);

    QString clientMaxHandshakeAttempts() const;
    void setClientMaxHandshakeAttempts(const QString &value);

    bool clientHeaderProtectionEnabled() const;

    bool clientRandomTrailers() const;
    void setClientRandomTrailers(bool value);

    bool clientDisableCookies() const;
    void setClientDisableCookies(bool value);

    QString serverJunkPacketCount() const;
    void setServerJunkPacketCount(const QString &value);

    QString serverJunkPacketMinSize() const;
    void setServerJunkPacketMinSize(const QString &value);

    QString serverJunkPacketMaxSize() const;
    void setServerJunkPacketMaxSize(const QString &value);

    QString serverInitPacketJunkSize() const;
    void setServerInitPacketJunkSize(const QString &value);

    QString serverResponsePacketJunkSize() const;
    void setServerResponsePacketJunkSize(const QString &value);

    QString serverCookieReplyPacketJunkSize() const;
    void setServerCookieReplyPacketJunkSize(const QString &value);

    QString serverTransportPacketJunkSize() const;
    void setServerTransportPacketJunkSize(const QString &value);

    QString serverInitPacketMagicHeader() const;
    void setServerInitPacketMagicHeader(const QString &value);

    QString serverResponsePacketMagicHeader() const;
    void setServerResponsePacketMagicHeader(const QString &value);

    QString serverUnderloadPacketMagicHeader() const;
    void setServerUnderloadPacketMagicHeader(const QString &value);

    QString serverTransportPacketMagicHeader() const;
    void setServerTransportPacketMagicHeader(const QString &value);

    QString serverSpecialJunk1() const;
    void setServerSpecialJunk1(const QString &value);

    QString serverSpecialJunk2() const;
    void setServerSpecialJunk2(const QString &value);

    QString serverSpecialJunk3() const;
    void setServerSpecialJunk3(const QString &value);

    QString serverSpecialJunk4() const;
    void setServerSpecialJunk4(const QString &value);

    QString serverSpecialJunk5() const;
    void setServerSpecialJunk5(const QString &value);

    QString serverContentPaddingAddition() const;
    void setServerContentPaddingAddition(const QString &value);

    QString serverRekeyAfterTime() const;
    void setServerRekeyAfterTime(const QString &value);

    QString serverRekeyTimeout() const;
    void setServerRekeyTimeout(const QString &value);

    QString serverRejectAfterTime() const;
    void setServerRejectAfterTime(const QString &value);

    QString serverKeepaliveTimeout() const;
    void setServerKeepaliveTimeout(const QString &value);

    QString serverMaxHandshakeAttempts() const;
    void setServerMaxHandshakeAttempts(const QString &value);

    bool serverHeaderProtectionEnabled() const;
    void setServerHeaderProtectionEnabled(bool value);

    bool serverRandomTrailers() const;
    void setServerRandomTrailers(bool value);

    bool serverDisableCookies() const;
    void setServerDisableCookies(bool value);

    bool isAwg2() const;

    bool isAwg3() const;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::AwgProtocolConfig &protocolConfig);
    amnezia::AwgProtocolConfig getProtocolConfig();
    bool isServerSettingsEqual();
    
    bool isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4);
    bool isPacketSizeEqual(const int s1, const int s2, const int s3, const int s4);

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    amnezia::AwgProtocolConfig m_protocolConfig;
    amnezia::AwgProtocolConfig m_originalProtocolConfig;
    
    QString serverProtocolVersion() const;
    void applyDefaultsToServerConfig(amnezia::AwgServerConfig& config);
    void applyDefaultsToClientConfig(amnezia::AwgClientConfig& config);
};

#endif // AWGCONFIGMODEL_H
