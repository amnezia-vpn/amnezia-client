#ifndef XRAYCONFIGMODEL_H
#define XRAYCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SiteRole,
        PortRole
    };

    explicit XrayConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const XrayProtocolConfig xrayProtocolConfig);
    QSharedPointer<ProtocolConfig> getConfig();

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    XrayProtocolConfig m_newXrayProtocolConfig;
    XrayProtocolConfig m_oldXrayProtocolConfig;
};

#endif // XRAYCONFIGMODEL_H
