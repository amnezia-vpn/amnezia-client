#include "apiBenefitsModel.h"

#include <QHash>
#include <utility>
#include <QJsonObject>
#include <QJsonValue>

#include "core/api/apiDefs.h"

namespace
{
namespace configKey
{
    constexpr char title[] = "title";
    constexpr char body[] = "body";
    constexpr char icon[] = "icon";
    constexpr char injectKey[] = "inject_key";
    constexpr char accent[] = "accent";

    constexpr char region[] = "region";
    constexpr char speed[] = "speed";
    constexpr char price[] = "price";
}

QString gatewayIconKeyToUrl(const QString &iconKey)
{
    if (iconKey.startsWith(QLatin1String("qrc:"))) {
        return iconKey;
    }
    static const QHash<QString, QString> map = {
        { QStringLiteral("globe-2"), QStringLiteral("qrc:/images/controls/globe-2.svg") },
        { QStringLiteral("smartphone"), QStringLiteral("qrc:/images/controls/smartphone.svg") },
        { QStringLiteral("gauge"), QStringLiteral("qrc:/images/controls/gauge.svg") },
        { QStringLiteral("infinity"), QStringLiteral("qrc:/images/controls/infinity.svg") },
        { QStringLiteral("tag"), QStringLiteral("qrc:/images/controls/tag.svg") },
        { QStringLiteral("history"), QStringLiteral("qrc:/images/controls/history.svg") },
        { QStringLiteral("info"), QStringLiteral("qrc:/images/controls/info.svg") },
        { QStringLiteral("app"), QStringLiteral("qrc:/images/controls/app.svg") },
        { QStringLiteral("download"), QStringLiteral("qrc:/images/controls/download.svg") },
        { QStringLiteral("help-circle"), QStringLiteral("qrc:/images/controls/help-circle.svg") },
    };
    return map.value(iconKey, QStringLiteral("qrc:/images/controls/info.svg"));
}
}

ApiBenefitsModel::ApiBenefitsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ApiBenefitsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_serviceBenefits.size();
}

QVariant ApiBenefitsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_serviceBenefits.size()) {
        return {};
    }
    const ServiceBenefitItem &item = m_serviceBenefits.at(index.row());
    switch (role) {
    case IconRole:
        return item.icon;
    case TitleRole:
        return item.title;
    case BodyRole:
        return item.body;
    case AccentRole:
        return item.accent;
    default:
        return {};
    }
}

QHash<int, QByteArray> ApiBenefitsModel::roleNames() const
{
    return {
        { IconRole, "icon" },
        { TitleRole, "title" },
        { BodyRole, "body" },
        { AccentRole, "accent" },
    };
}

void ApiBenefitsModel::updateModel(const QJsonArray &benefits, const QString &region, const QString &speed,
                                   const QString &price, const QJsonObject &supportInfo)
{
    beginResetModel();
    m_serviceBenefits.clear();
    for (const QJsonValue &benefitValue : benefits) {
        if (!benefitValue.isObject()) {
            continue;
        }
        const QJsonObject benefitObject = benefitValue.toObject();
        QString title = benefitObject.value(configKey::title).toString();
        QString body = benefitObject.value(configKey::body).toString();
        const QString iconKey = benefitObject.value(configKey::icon).toString();
        const QString injectKey = benefitObject.value(configKey::injectKey).toString();
        if (body.contains(QLatin1String("%1")) && !injectKey.isEmpty()) {
            QString injected = benefitInjectValue(injectKey, region, speed, price, supportInfo);
            if (injected.isEmpty()) {
                injected = QStringLiteral("—");
            }
            body = body.arg(injected);
        }
        if (title.isEmpty() && body.isEmpty()) {
            continue;
        }
        ServiceBenefitItem item;
        item.icon = gatewayIconKeyToUrl(iconKey);
        item.title = std::move(title);
        item.body = std::move(body);
        item.accent = benefitObject.value(configKey::accent).toBool();
        m_serviceBenefits.append(std::move(item));
    }
    endResetModel();
}

void ApiBenefitsModel::clear()
{
    beginResetModel();
    m_serviceBenefits.clear();
    endResetModel();
}

QString ApiBenefitsModel::formatPriceForBenefit(const QString &rawPrice) const
{
    if (rawPrice == QStringLiteral("free")) {
        return tr("Free");
    }
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    return tr("%1 $").arg(rawPrice);
#else
    return tr("%1 $/month").arg(rawPrice);
#endif
}

QString ApiBenefitsModel::benefitInjectValue(const QString &injectKey, const QString &region, const QString &speed,
                                               const QString &price, const QJsonObject &supportInfo) const
{
    if (injectKey == QLatin1String(configKey::region)) {
        return region.isEmpty() ? QStringLiteral("—") : region;
    }
    if (injectKey == QLatin1String(configKey::speed)) {
        return speed.isEmpty() ? QStringLiteral("—") : speed;
    }
    if (injectKey == QLatin1String(configKey::price)) {
        return formatPriceForBenefit(price);
    }

    if (injectKey == apiDefs::key::telegram) {
        const QString handle = supportInfo.value(apiDefs::key::telegram).toString().trimmed();
        if (handle.isEmpty()) {
            return QStringLiteral("—");
        }
        if (handle.startsWith(QLatin1Char('@'))) {
            return handle;
        }
        return QLatin1Char('@') + handle;
    }
    return QString();
}
