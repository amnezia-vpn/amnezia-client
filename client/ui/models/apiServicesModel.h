#ifndef APISERVICESMODEL_H
#define APISERVICESMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>

class ApiServicesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        PriceRole
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
    QString getCountryCode();
    QString getServicesDescription();
signals:

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QString m_countryCode;
    QString m_servicesDescription;
    QJsonArray m_services;

    int m_selectedServiceIndex;
};

#endif // APISERVICESMODEL_H
