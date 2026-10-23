#ifndef SOCKS5PROXYCONFIGMODEL_H
#define SOCKS5PROXYCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"

class Socks5ProxyConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        UserNameRole,
        PasswordRole
    };

    explicit Socks5ProxyConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::Socks5ProxyProtocolConfig &protocolConfig);
    amnezia::Socks5ProxyProtocolConfig getProtocolConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::Socks5ProxyProtocolConfig m_protocolConfig;
};

#endif // SOCKS5PROXYCONFIGMODEL_H
