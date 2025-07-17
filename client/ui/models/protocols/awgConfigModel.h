#ifndef AWGCONFIGMODEL_H
#define AWGCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"

namespace AwgConstant
{
    const int messageInitiationSize = 148;
    const int messageResponseSize = 92;
    const int messageCookieReplySize = 64;
    const int messageTransportSize = 32;
}

struct AwgConfig
{
    AwgConfig(const QJsonObject &jsonConfig);

    QString subnetAddress;
    QString port;

    QString clientMtu;
    QString clientJunkPacketCount;
    QString clientJunkPacketMinSize;
    QString clientJunkPacketMaxSize;
    QString clientSpecialJunk1;
    QString clientSpecialJunk2;
    QString clientSpecialJunk3;
    QString clientSpecialJunk4;
    QString clientSpecialJunk5;
    QString clientControlledJunk1;
    QString clientControlledJunk2;
    QString clientControlledJunk3;
    QString clientSpecialHandshakeTimeout;

    QString serverJunkPacketCount;
    QString serverJunkPacketMinSize;
    QString serverJunkPacketMaxSize;
    QString serverInitPacketJunkSize;
    QString serverResponsePacketJunkSize;
    QString serverCookieReplyPacketJunkSize;
    QString serverTransportPacketJunkSize;
    QString serverInitPacketMagicHeader;
    QString serverResponsePacketMagicHeader;
    QString serverUnderloadPacketMagicHeader;
    QString serverTransportPacketMagicHeader;

    bool hasEqualServerSettings(const AwgConfig &other) const;
    bool hasEqualClientSettings(const AwgConfig &other) const;
};

class AwgConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SubnetAddressRole = Qt::UserRole + 1,
        PortRole,

        ClientMtuRole,
        ClientJunkPacketCountRole,
        ClientJunkPacketMinSizeRole,
        ClientJunkPacketMaxSizeRole,
        ClientSpecialJunk1Role,
        ClientSpecialJunk2Role,
        ClientSpecialJunk3Role,
        ClientSpecialJunk4Role,
        ClientSpecialJunk5Role,
        ClientControlledJunk1Role,
        ClientControlledJunk2Role,
        ClientControlledJunk3Role,
        ClientSpecialHandshakeTimeoutRole,

        ServerJunkPacketCountRole,
        ServerJunkPacketMinSizeRole,
        ServerJunkPacketMaxSizeRole,
        ServerInitPacketJunkSizeRole,
        ServerResponsePacketJunkSizeRole,
        ServerCookieReplyPacketJunkSizeRole,
        ServerTransportPacketJunkSizeRole,

        ServerInitPacketMagicHeaderRole,
        ServerResponsePacketMagicHeaderRole,
        ServerUnderloadPacketMagicHeaderRole,
        ServerTransportPacketMagicHeaderRole,
    };

    explicit AwgConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonObject &config);
    QJsonObject getConfig();

    bool isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4);
    bool isPacketSizeEqual(const int s1, const int s2/*, const int s3, const int s4*/);

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    DockerContainer m_container;
    QJsonObject m_serverProtocolConfig;
    QJsonObject m_clientProtocolConfig;
    QJsonObject m_fullConfig;
};

#endif // AWGCONFIGMODEL_H
