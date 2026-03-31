#include "apiSubscriptionPlansModel.h"

#include <QJsonObject>
#include <QJsonValue>
#include <utility>

namespace
{
namespace configKey
{
    constexpr char primaryLeft[] = "primary_left";
    constexpr char primaryRight[] = "primary_right";
    constexpr char subtitle[] = "subtitle";
    constexpr char recommended[] = "recommended";
    constexpr char checkoutUrl[] = "checkout_url";
    constexpr char isTrial[] = "is_trial";
    constexpr char serviceProtocol[] = "service_protocol";
    constexpr char storeProductId[] = "store_product_id";

    constexpr char primaryLeftCamel[] = "primaryLeft";
    constexpr char primaryRightCamel[] = "primaryRight";
    constexpr char checkoutUrlCamel[] = "checkoutUrl";
    constexpr char isTrialCamel[] = "isTrial";
    constexpr char serviceProtocolCamel[] = "serviceProtocol";
    constexpr char storeProductIdCamel[] = "storeProductId";
}
}

ApiSubscriptionPlansModel::ApiSubscriptionPlansModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ApiSubscriptionPlansModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_subscriptionPlans.size();
}

QVariant ApiSubscriptionPlansModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_subscriptionPlans.size()) {
        return {};
    }
    const SubscriptionPlanItem &plan = m_subscriptionPlans.at(index.row());
    switch (role) {
    case PrimaryLeftRole:
        return plan.primaryLeft;
    case PrimaryRightRole:
        return plan.primaryRight;
    case SubtitleRole:
        return plan.subtitle;
    case RecommendedRole:
        return plan.recommended;
    case CheckoutUrlRole:
        return plan.checkoutUrl;
    case IsTrialRole:
        return plan.isTrial;
    case ServiceProtocolRole:
        return plan.serviceProtocol;
    case StoreProductIdRole:
        return plan.storeProductId;
    default:
        return {};
    }
}

QHash<int, QByteArray> ApiSubscriptionPlansModel::roleNames() const
{
    return {
        { PrimaryLeftRole, "primaryLeft" },
        { PrimaryRightRole, "primaryRight" },
        { SubtitleRole, "subtitle" },
        { RecommendedRole, "recommended" },
        { CheckoutUrlRole, "checkoutUrl" },
        { IsTrialRole, "isTrial" },
        { ServiceProtocolRole, "serviceProtocol" },
        { StoreProductIdRole, "storeProductId" },
    };
}

void ApiSubscriptionPlansModel::updateModel(const QJsonArray &arr)
{
    beginResetModel();
    m_subscriptionPlans.clear();
    m_subscriptionPlans.reserve(arr.size());
    for (const QJsonValue &planValue : arr) {
        if (!planValue.isObject()) {
            continue;
        }
        const QJsonObject planObject = planValue.toObject();
        SubscriptionPlanItem subscriptionPlan;
        subscriptionPlan.primaryLeft = planObject.value(configKey::primaryLeft).toString();
        subscriptionPlan.primaryRight = planObject.value(configKey::primaryRight).toString();
        subscriptionPlan.subtitle = planObject.value(configKey::subtitle).toString();
        subscriptionPlan.recommended = planObject.value(configKey::recommended).toBool();
        subscriptionPlan.checkoutUrl = planObject.value(configKey::checkoutUrl).toString();
        subscriptionPlan.isTrial = planObject.value(configKey::isTrial).toBool();
        subscriptionPlan.serviceProtocol = planObject.value(configKey::serviceProtocol).toString();
        subscriptionPlan.storeProductId = planObject.value(configKey::storeProductId).toString();
        m_subscriptionPlans.append(std::move(subscriptionPlan));
    }
    endResetModel();
}

void ApiSubscriptionPlansModel::clear()
{
    beginResetModel();
    m_subscriptionPlans.clear();
    endResetModel();
}

QVariantMap ApiSubscriptionPlansModel::planAt(int row) const
{
    if (row < 0 || row >= m_subscriptionPlans.size()) {
        return {};
    }
    const SubscriptionPlanItem &plan = m_subscriptionPlans.at(row);
    QVariantMap planMap;
    planMap.insert(QLatin1String(configKey::primaryLeftCamel), plan.primaryLeft);
    planMap.insert(QLatin1String(configKey::primaryRightCamel), plan.primaryRight);
    planMap.insert(QLatin1String(configKey::subtitle), plan.subtitle);
    planMap.insert(QLatin1String(configKey::recommended), plan.recommended);
    planMap.insert(QLatin1String(configKey::checkoutUrlCamel), plan.checkoutUrl);
    planMap.insert(QLatin1String(configKey::isTrialCamel), plan.isTrial);
    planMap.insert(QLatin1String(configKey::serviceProtocolCamel), plan.serviceProtocol);
    planMap.insert(QLatin1String(configKey::storeProductIdCamel), plan.storeProductId);
    return planMap;
}

int ApiSubscriptionPlansModel::recommendedRowIndex() const
{
    for (int planIndex = 0; planIndex < m_subscriptionPlans.size(); ++planIndex) {
        if (m_subscriptionPlans.at(planIndex).recommended) {
            return planIndex;
        }
    }
    return 0;
}
