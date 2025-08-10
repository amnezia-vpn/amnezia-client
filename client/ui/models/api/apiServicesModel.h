#ifndef APISERVICESMODEL_H
#define APISERVICESMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>

class ApiServicesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        CardDescriptionRole,
        ServiceDescriptionRole,
        IsServiceAvailableRole,
        SpeedRole,
        TimeLimitRole,
        RegionRole,
        FeaturesRole,
        PriceRole,
        EndDateRole
    };

    explicit ApiServicesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonObject &data);

    void setServiceIndex(const int index);

    QJsonObject getSelectedServiceInfo();
    QString getSelectedServiceType();
    QString getSelectedServiceProtocol();
    QString getSelectedServiceName();
    QJsonArray getSelectedServiceCountries();

    QString getCountryCode();

    QString getStoreEndpoint();

    QVariant getSelectedServiceData(const QString roleString);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    struct ServiceInfo
    {
        QString name;
        QString speed;
        QString timeLimit;
        QString region;
        QString price;

        QJsonObject object;
    };

    struct Subscription
    {
        QString endDate;
    };

    struct ApiServicesData
    {
        bool isServiceAvailable;

        QString type;
        QString protocol;
        QString storeEndpoint;

        ServiceInfo serviceInfo;
        Subscription subscription;

        QJsonArray availableCountries;
    };

    ApiServicesData getApiServicesData(const QJsonObject &data);

    QString m_countryCode;
    QVector<ApiServicesData> m_services;

    int m_selectedServiceIndex;
};

#endif // APISERVICESMODEL_H
