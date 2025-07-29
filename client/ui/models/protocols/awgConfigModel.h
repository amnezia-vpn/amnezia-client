#ifndef AWGCONFIGMODEL_H
#define AWGCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/models/protocols/awgProtocolConfig.h"

namespace AwgConstant
{
    const int messageInitiationSize = 148;
    const int messageResponseSize = 92;
    const int messageCookieReplySize = 64;
    const int messageTransportSize = 32;
}

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
    void updateModel(const AwgProtocolConfig awgProtocolConfig);
    QSharedPointer<ProtocolConfig> getConfig();

    bool isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4);
    bool isPacketSizeEqual(const int s1, const int s2/*, const int s3, const int s4*/);

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    AwgProtocolConfig m_newAwgProtocolConfig;
    AwgProtocolConfig m_oldAwgProtocolConfig;
};

#endif // AWGCONFIGMODEL_H
