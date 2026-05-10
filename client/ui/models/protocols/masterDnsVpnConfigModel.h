#ifndef MASTERDNSVPNCONFIGMODEL_H
#define MASTERDNSVPNCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/models/protocols/masterDnsVpnProtocolConfig.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"

class MasterDnsVpnConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        DomainsRole,
        PortRole,
        EncryptionMethodRole,
        ProtocolTypeRole,
        ListenPortRole
    };

    explicit MasterDnsVpnConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(amnezia::DockerContainer container,
                     const amnezia::MasterDnsVpnProtocolConfig &protocolConfig);
    amnezia::MasterDnsVpnProtocolConfig getProtocolConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::MasterDnsVpnProtocolConfig m_protocolConfig;
    amnezia::MasterDnsVpnProtocolConfig m_originalProtocolConfig;

    void applyDefaultsToServerConfig(amnezia::MasterDnsVpnServerConfig &config);
};

#endif // MASTERDNSVPNCONFIGMODEL_H
