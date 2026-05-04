#ifndef PROTOCOLS_MODEL_H
#define PROTOCOLS_MODEL_H

#include <QAbstractListModel>

#include "../controllers/qml/pageController.h"
#include "core/models/containerConfig.h"

class ProtocolsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ProtocolNameRole = Qt::UserRole + 1,
        ServerProtocolPageRole,
        ClientProtocolPageRole,
        ProtocolIndexRole,
        ProtocolStringRole,
        RawConfigRole,
        IsClientProtocolExistsRole,
        // Protocol type check roles
        IsWireGuardRole,
        IsAwgRole,
        IsOpenVpnRole,
        IsXrayRole,
        IsSftpRole,
        IsIpsecRole,
        IsSocks5ProxyRole,
        IsMtProxyRole,
    };

    explicit ProtocolsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const amnezia::ContainerConfig &containerConfig);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    PageLoader::PageEnum serverProtocolPage(Proto protocol) const;
    PageLoader::PageEnum clientProtocolPage(Proto protocol) const;
    Proto getProtocolType() const;
    QString getRawConfig() const;
    bool isClientProtocolExists() const;

    amnezia::ContainerConfig m_containerConfig;
};

#endif // PROTOCOLS_MODEL_H
