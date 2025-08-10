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

    explicit ApiCountryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public slots:
    void updateModel(const QJsonArray &countries, const QString &currentCountryCode);
    void updateIssuedConfigsInfo(const QJsonArray &issuedConfigs);

    int getCurrentIndex();
    void setCurrentIndex(const int i);

signals:
    void currentIndexChanged(const int index);

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

    struct CountryInfo
    {
        QString countryName;
        QString countryCode;
    };

    QVector<CountryInfo> m_countries;
    QHash<QString, IssuedConfigInfo> m_issuedConfigs;
    int m_currentIndex;
};

#endif // APICOUNTRYMODEL_H
