#ifndef WIREGUARDCONFIGMODEL_H
#define WIREGUARDCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/models/protocols/wireguardProtocolConfig.h"

class WireGuardConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SubnetAddressRole = Qt::UserRole + 1,
        PortRole,
        ClientMtuRole
    };

    explicit WireGuardConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const WireGuardProtocolConfig wireGuardProtocolConfig);
    QSharedPointer<ProtocolConfig> getConfig();

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    WireGuardProtocolConfig m_newWireGuardProtocolConfig;
    WireGuardProtocolConfig m_oldWireGuardProtocolConfig;
};

#endif // WIREGUARDCONFIGMODEL_H
