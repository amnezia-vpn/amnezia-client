#ifndef CONTAINERS_MODEL_H
#define CONTAINERS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <vector>
#include <utility>

#include "settings.h"
#include "containers/containers_defs.h"

class ContainersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ContainersModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescRole,
        ServiceTypeRole,
        ConfigRole,
        DockerContainerRole,

        IsEasySetupContainerRole,
        EasySetupHeaderRole,
        EasySetupDescriptionRole,

        IsInstalledRole,
        IsCurrentlyProcessedRole,
        IsDefaultRole,
        IsSupportedRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

signals:
    void defaultContainerChanged();

public slots:
    DockerContainer getDefaultContainer();
    QString getDefaultContainerName();

    void setCurrentlyProcessedServerIndex(int index);
    void setCurrentlyProcessedContainerIndex(int index);
    int getCurrentlyProcessedContainerIndex();

    void removeAllContainers();
    void clearCachedProfiles();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QMap<DockerContainer, QJsonObject> m_containers;


    int m_currentlyProcessedServerIndex;
    int m_currentlyProcessedContainerIndex;
    DockerContainer m_defaultContainerIndex;

    std::shared_ptr<Settings> m_settings;
};

#endif // CONTAINERS_MODEL_H
