#ifndef APICOUNTRYMODEL_H
#define APICOUNTRYMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>

class ApiCountryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        CountryNameRole = Qt::UserRole + 1,
        CountryCodeRole,
        CountryImageCodeRole
    };

    explicit ApiCountryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public slots:
    void updateModel(const QJsonArray &data, const QString &currentCountryCode);

    int getCurrentIndex();
    void setCurrentIndex(const int i);

signals:
    void currentIndexChanged(const int index);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QJsonArray m_countries;
    int m_currentIndex;
};

#endif // APICOUNTRYMODEL_H
