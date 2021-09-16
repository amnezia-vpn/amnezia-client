#ifndef PROTOCOLS_MODEL_H
#define PROTOCOLS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <vector>
#include <utility>

#include "settings.h"
#include "containers/containers_defs.h"

class ProtocolsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ProtocolsModel(QObject *parent = nullptr);
public:
    enum SiteRoles {
        NameRole = Qt::UserRole + 1,
        DescRole,
        isVpnTypeRole,
        isOtherTypeRole,
        isInstalledRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE void setSelectedServerIndex(int index);
    Q_INVOKABLE void setSelectedDockerContainer(amnezia::DockerContainer c);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    int m_selectedServerIndex;
    DockerContainer m_selectedDockerContainer;
    Settings m_settings;
};

#endif // PROTOCOLS_MODEL_H
