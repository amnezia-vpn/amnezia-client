#ifndef CONTAINERS_MODEL_H
#define CONTAINERS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <utility>
#include <vector>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"

class ContainersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ContainersModel(QObject *parent = nullptr);

    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        DetailedDescriptionRole,
        ServiceTypeRole,
        ConfigRole,
        IsThirdPartyConfigRole,
        DockerContainerRole,
        ContainerStringRole,

        IsEasySetupContainerRole,
        EasySetupHeaderRole,
        EasySetupDescriptionRole,
        EasySetupOrderRole,

        IsInstallationAllowedRole,
        IsInstalledRole,
        IsCurrentlyProcessedRole,
        IsDefaultRole,
        IsSupportedRole,
        IsShareableRole,

        IsUnsupportedContainerRole,

        InstallPageOrderRole,
        
        // Container type check roles
        IsVpnContainerRole,
        IsServiceContainerRole,
        IsIpsecRole,
        IsDnsRole,
        IsSftpRole,
        IsTorWebsiteRole,
        IsSocks5ProxyRole,
        IsMtProxyRole,
        IsTelemtRole,
    };
    
    Q_INVOKABLE void openContainerSettings(int containerIndex);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role) const;

public slots:
    void updateModel(const QMap<amnezia::DockerContainer, amnezia::ContainerConfig> &containers);

    void setProcessedContainerIndex(int containerIndex);

    QString getProcessedContainerName();

    QJsonObject getContainerConfig(const int containerIndex);

    bool isSupportedByCurrentPlatform(const int containerIndex);
    bool isServiceContainer(const int containerIndex);

    bool hasInstalledServices();
    bool hasInstalledProtocols();

    static bool isInstallationAllowed(amnezia::DockerContainer container);

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void containersModelUpdated();

private:
    QMap<amnezia::DockerContainer, amnezia::ContainerConfig> m_containers;

    int m_processedContainerIndex = -1;
};

#endif // CONTAINERS_MODEL_H
