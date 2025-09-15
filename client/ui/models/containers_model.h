#ifndef CONTAINERS_MODEL_H
#define CONTAINERS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/models/containers/containerConfig.h"

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
        DockerContainerRole,

        IsEasySetupContainerRole,
        EasySetupHeaderRole,
        EasySetupDescriptionRole,
        EasySetupOrderRole,

        IsInstalledRole,
        IsCurrentlyProcessedRole,
        IsDefaultRole,
        IsSupportedRole,
        IsShareableRole,

        InstallPageOrderRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role) const;

public slots:
    void updateModel(const QMap<QString, ContainerConfig> &containerConfigs);

    void setProcessedContainerIndex(int containerIndex);
    int getProcessedContainerIndex();

    QString getProcessedContainerName();

    bool isSupportedByCurrentPlatform(const int containerIndex);
    bool isServiceContainer(const int containerIndex);

    bool hasInstalledServices();
    bool hasInstalledProtocols();

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void containersModelUpdated();

private:
    QMap<QString, ContainerConfig> m_containerConfigs;

    int m_processedContainerIndex;
};

#endif // CONTAINERS_MODEL_H
