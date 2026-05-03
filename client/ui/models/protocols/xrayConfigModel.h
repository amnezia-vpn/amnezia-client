#ifndef XRAYCONFIGMODEL_H
#define XRAYCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SiteRole,
        PortRole
    };

    explicit XrayConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig &protocolConfig);
    amnezia::XrayProtocolConfig getProtocolConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::XrayProtocolConfig m_protocolConfig;
    amnezia::XrayProtocolConfig m_originalProtocolConfig;
    
    void applyDefaultsToServerConfig(amnezia::XrayServerConfig& config);
};

#endif // XRAYCONFIGMODEL_H
