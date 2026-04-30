#ifndef APISERVICESMODEL_H
#define APISERVICESMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

class ApiServicesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct ServiceInfo
    {
        QString name;

        QString description;
        QString cardDescription;

        QString termsOfUseUrl;
        QString privacyPolicyUrl;

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
        QJsonObject supportInfo;
        Subscription subscription;

        QJsonArray availableCountries;

        QJsonArray subscriptionPlansJson;
        QJsonArray benefits;

        QString minPriceLabel;
    };

    enum Roles {
        NameRole = Qt::UserRole + 1,
        CardDescriptionRole,
        ServiceDescriptionRole,
        IsServiceAvailableRole,
        IsPremiumRole,
        HasSubscriptionPlansRole,
        PriceRole,
        EndDateRole,
        TermsOfUseUrlRole,
        PrivacyPolicyUrlRole,
        ShowRecommendedRole,
        OrderRole
    };

    explicit ApiServicesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ApiServicesData selectedServiceData() const;

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

    Q_INVOKABLE int serviceIndexForType(const QString &type) const;

signals:
    void serviceSelectionChanged();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    ApiServicesData getApiServicesData(const QJsonObject &data);

    QString m_countryCode;
    QVector<ApiServicesData> m_services;

    int m_selectedServiceIndex;
};

#endif
