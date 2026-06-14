#ifndef IKEV2CONFIGMODEL_H
#define IKEV2CONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"

class Ikev2ConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        CipherRole
    };

    explicit Ikev2ConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::Ikev2ProtocolConfig &protocolConfig);
    amnezia::Ikev2ProtocolConfig getProtocolConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::Ikev2ProtocolConfig m_protocolConfig;
    amnezia::Ikev2ProtocolConfig m_originalProtocolConfig;
};

#endif // IKEV2CONFIGMODEL_H
