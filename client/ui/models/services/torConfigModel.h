#ifndef TORCONFIGMODEL_H
#define TORCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/torProtocolConfig.h"

class TorConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SiteRole
    };

    explicit TorConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::TorProtocolConfig &protocolConfig);
    amnezia::TorProtocolConfig getProtocolConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::TorProtocolConfig m_protocolConfig;
    amnezia::TorProtocolConfig m_originalProtocolConfig;
};

#endif // TORCONFIGMODEL_H

