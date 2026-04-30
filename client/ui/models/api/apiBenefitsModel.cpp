#include "apiBenefitsModel.h"

#include <QHash>
#include <utility>
#include <QJsonObject>
#include <QJsonValue>

namespace
{
namespace configKey
{
    constexpr char title[] = "title";
    constexpr char body[] = "body";
    constexpr char icon[] = "icon";
    constexpr char link[] = "link";
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
    case LinkRole:
        return item.link;
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
        { LinkRole, "link" },
    };
}

void ApiBenefitsModel::updateModel(const QJsonArray &benefits)
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
        const bool isLink = benefitObject.value(configKey::link).toBool();
        const QString iconKey = benefitObject.value(configKey::icon).toString();
        if (isLink) {
            body = body.trimmed();
        }
        if (title.isEmpty() && body.isEmpty()) {
            continue;
        }
        ServiceBenefitItem item;
        item.icon = gatewayIconKeyToUrl(iconKey);
        item.title = std::move(title);
        item.body = std::move(body);
        item.link = isLink;
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
