#ifndef APISUBSCRIPTIONPLANSMODEL_H
#define APISUBSCRIPTIONPLANSMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QVector>

class ApiSubscriptionPlansModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        BillingPeriodRole = Qt::UserRole + 1,
        PriceLabelRole,
        SubtitleRole,
        RecommendedRole,
        CheckoutUrlRole,
        IsTrialRole,
        ServiceProtocolRole,
        StoreProductIdRole
    };
    Q_ENUM(Roles)

    explicit ApiSubscriptionPlansModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateModel(const QJsonArray &arr);
    void clear();

    Q_INVOKABLE QVariantMap planAt(int row) const;
    Q_INVOKABLE int recommendedRowIndex() const;

private:
    struct SubscriptionPlanItem
    {
        QString billingPeriod;
        QString priceLabel;
        QString subtitle;
        bool recommended = false;
        QString checkoutUrl;
        bool isTrial = false;
        QString serviceProtocol;
        QString storeProductId;
    };

    QVector<SubscriptionPlanItem> m_subscriptionPlans;
};

#endif
