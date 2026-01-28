#ifndef WIREGUARDCONFIGMODEL_H
#define WIREGUARDCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"

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
    void updateModel(amnezia::DockerContainer container, const amnezia::WireGuardProtocolConfig &protocolConfig);
    amnezia::WireGuardProtocolConfig getProtocolConfig();

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::WireGuardProtocolConfig m_protocolConfig;
    amnezia::WireGuardProtocolConfig m_originalProtocolConfig;
    
    void applyDefaultsToServerConfig(amnezia::WireGuardServerConfig& config);
    void applyDefaultsToClientConfig(amnezia::WireGuardClientConfig& config);
};

#endif // WIREGUARDCONFIGMODEL_H
