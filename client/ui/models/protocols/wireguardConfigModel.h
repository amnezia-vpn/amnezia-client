#ifndef WIREGUARDCONFIGMODEL_H
#define WIREGUARDCONFIGMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "containers/containers_defs.h"

struct WgConfig
{
    WgConfig(const QJsonObject &jsonConfig);

    QString subnetAddress;
    QString port;
    QString clientMtu;

    bool hasEqualServerSettings(const WgConfig &other) const;
    bool hasEqualClientSettings(const WgConfig &other) const;

};

class WireGuardConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SubnetAddressRole = Qt::UserRole + 1,
        PortRole,
        ClientMtuRole
    };

    explicit WireGuardConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonObject &config);
    QJsonObject getConfig();

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    DockerContainer m_container;
    QJsonObject m_serverProtocolConfig;
    QJsonObject m_clientProtocolConfig;
    QJsonObject m_fullConfig;
};

#endif // WIREGUARDCONFIGMODEL_H
