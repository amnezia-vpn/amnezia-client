#ifndef CONTAINERS_MODEL_H
#define CONTAINERS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <utility>
#include <vector>

#include "containers/containers_defs.h"

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
        DockerContainerRole,

        IsEasySetupContainerRole,
        EasySetupHeaderRole,
        EasySetupDescriptionRole,
        EasySetupOrderRole,

        IsInstalledRole,
        IsCurrentlyProcessedRole,
        IsDefaultRole,
        IsSupportedRole,
        IsShareableRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role) const;

public slots:
    void updateModel(const QJsonArray &containers);

    void setCurrentlyProcessedContainerIndex(int containerIndex);
    int getCurrentlyProcessedContainerIndex();

    QString getCurrentlyProcessedContainerName();

    QJsonObject getContainerConfig(const int containerIndex);

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void containersModelUpdated();

private:
    QMap<DockerContainer, QJsonObject> m_containers;

    int m_currentlyProcessedContainerIndex;
};

#endif // CONTAINERS_MODEL_H
