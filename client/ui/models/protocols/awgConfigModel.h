#ifndef AWGCONFIGMODEL_H
#define AWGCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"

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

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    DockerContainer m_container;
    QJsonObject m_protocolConfig;
    QJsonObject m_fullConfig;
};

#endif // AWGCONFIGMODEL_H
