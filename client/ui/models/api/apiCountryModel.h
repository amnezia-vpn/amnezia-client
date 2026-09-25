#ifndef APICOUNTRYMODEL_H
#define APICOUNTRYMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QJsonArray>

class ApiCountryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CountryNameRole = Qt::UserRole + 1,
        CountryCodeRole,
        CountryImageCodeRole,
        IsIssuedRole,
        IsWorkerExpiredRole
    };

    struct CountryInfo
    {
        QString countryName;
        QString countryCode;
        QString countryCodeL10n;
    };

    explicit ApiCountryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const QVector<CountryInfo> &countries() const;

    Q_PROPERTY(int currentIndex READ getCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentCountryCode READ getCurrentCountryCode NOTIFY currentIndexChanged)
    Q_PROPERTY(bool hasExpiredWorkerConfigs READ hasExpiredWorkerConfigs NOTIFY issuedConfigsChanged)

public slots:
    void updateModel(const QJsonArray &countries, const QString &currentCountryCode,
                     const QString &userCountryCode);
    void updateIssuedConfigsInfo(const QJsonArray &issuedConfigs);

    QString getCurrentCountryCode() const;
    QString getUserCountryCode() const;
    int getCurrentIndex();

    bool hasExpiredWorkerConfigs() const;

signals:
    void currentIndexChanged(const int index);
    void issuedConfigsChanged();

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
    };

    bool isWorkerExpired(const IssuedConfigInfo &issuedConfigInfo) const;

    QVector<CountryInfo> m_countries;
    QHash<QString, IssuedConfigInfo> m_issuedConfigs;
    QString m_currentCountryCode;
    QString m_userCountryCode;
    int m_currentIndex = -1;
};

#endif // APICOUNTRYMODEL_H
