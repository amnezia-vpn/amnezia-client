#ifndef APIBENEFITSMODEL_H
#define APIBENEFITSMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
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
        AccentRole
    };
    Q_ENUM(Roles)

    explicit ApiBenefitsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateModel(const QJsonArray &benefits, const QString &region, const QString &speed, const QString &price,
                     const QJsonObject &supportInfo);
    void clear();

private:
    struct ServiceBenefitItem
    {
        QString icon;
        QString title;
        QString body;
        bool accent = false;
    };

    QVector<ServiceBenefitItem> m_serviceBenefits;

    QString formatPriceForBenefit(const QString &rawPrice) const;
    QString benefitInjectValue(const QString &injectKey, const QString &region, const QString &speed, const QString &price,
                               const QJsonObject &supportInfo) const;
};

#endif
