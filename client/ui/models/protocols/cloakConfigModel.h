#ifndef CLOAKCONFIGMODEL_H
#define CLOAKCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/models/protocols/cloakProtocolConfig.h"

class CloakConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        CipherRole,
        SiteRole
    };

    explicit CloakConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const CloakProtocolConfig cloakProtocolConfig);
    QSharedPointer<ProtocolConfig> getConfig();

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    CloakProtocolConfig m_newCloakProtocolConfig;
    CloakProtocolConfig m_oldCloakProtocolConfig;
};

#endif // CLOAKCONFIGMODEL_H
