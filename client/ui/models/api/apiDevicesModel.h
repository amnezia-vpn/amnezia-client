#ifndef APIDEVICESMODEL_H
#define APIDEVICESMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QVector>

#include "settings.h"

class ApiDevicesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        OsVersionRole = Qt::UserRole + 1,
        SupportTagRole,
        CountryCodeRole,
        LastUpdateRole,
        IsCurrentDeviceRole
    };

    explicit ApiDevicesModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonArray &issuedConfigs);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    struct IssuedConfigInfo
    {
        QString installationUuid;
        QString workerLastUpdated;
        QString lastDownloaded;
        QString sourceType;
        QString osVersion;

        QString countryName;
        QString countryCode;
    };

    QVector<IssuedConfigInfo> m_issuedConfigs;

    std::shared_ptr<Settings> m_settings;
};
#endif // APIDEVICESMODEL_H
