#ifndef AWGCONFIGMODEL_H
#define AWGCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"

namespace AwgConstant {
    const int messageInitiationSize = 148;
    const int messageResponseSize = 92;
}

struct AwgConfig
{
    AwgConfig(const QJsonObject &jsonConfig);

    QString port;
    QString mtu;
    QString junkPacketCount;
    QString junkPacketMinSize;
    QString junkPacketMaxSize;
    QString initPacketJunkSize;
    QString responsePacketJunkSize;
    QString initPacketMagicHeader;
    QString responsePacketMagicHeader;
    QString underloadPacketMagicHeader;
    QString transportPacketMagicHeader;

    bool hasEqualServerSettings(const AwgConfig &other) const;
    bool hasEqualClientSettings(const AwgConfig &other) const;

};

class AwgConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        MtuRole,
        JunkPacketCountRole,
        JunkPacketMinSizeRole,
        JunkPacketMaxSizeRole,
        InitPacketJunkSizeRole,
        ResponsePacketJunkSizeRole,
        InitPacketMagicHeaderRole,
        ResponsePacketMagicHeaderRole,
        UnderloadPacketMagicHeaderRole,
        TransportPacketMagicHeaderRole
    };

    explicit AwgConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonObject &config);
    QJsonObject getConfig();

    bool isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4);
    bool isPacketSizeEqual(const int s1, const int s2);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    DockerContainer m_container;
    QJsonObject m_protocolConfig;
    QJsonObject m_fullConfig;
};

#endif // AWGCONFIGMODEL_H
