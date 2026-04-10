#ifndef APIBENEFITSMODEL_H
#define APIBENEFITSMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QString>
#include <QVector>

class ApiBenefitsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IconRole = Qt::UserRole + 1,
        TitleRole,
        BodyRole,
        LinkRole
    };
    Q_ENUM(Roles)

    explicit ApiBenefitsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateModel(const QJsonArray &benefits);
    void clear();

private:
    struct ServiceBenefitItem
    {
        QString icon;
        QString title;
        QString body;
        bool link = false;
    };

    QVector<ServiceBenefitItem> m_serviceBenefits;
};

#endif
