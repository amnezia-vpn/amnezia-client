#include "utils.h"

#include <QDateTime>

namespace qqsfpm {

int compareVariants(const QVariant &lhs, const QVariant &rhs)
{
    // Do the QString check first because otherwise the canConvert<int> check will get hit for strings.
    if (lhs.typeId() == QMetaType::QString && rhs.typeId() == QMetaType::QString) {
        const auto lhsValue = lhs.toString();
        const auto rhsValue = rhs.toString();
        if (lhsValue == rhsValue)
            return 0;
        return lhsValue.compare(rhsValue, Qt::CaseInsensitive);
    } else if (lhs.typeId() == QMetaType::Bool && rhs.typeId() == QMetaType::Bool) {
        const auto lhsValue = lhs.toBool();
        const auto rhsValue = rhs.toBool();
        if (lhsValue == rhsValue)
            return 0;
        // false < true.
        return !lhsValue ? -1 : 1;
    } else if (lhs.typeId() == QMetaType::QDate && rhs.typeId() == QMetaType::QDate) {
        const auto lhsValue = lhs.toDate();
        const auto rhsValue = rhs.toDate();
        if (lhsValue == rhsValue)
            return 0;
        return lhsValue < rhsValue ? -1 : 1;
    } else if (lhs.typeId() == QMetaType::QDateTime && rhs.typeId() == QMetaType::QDateTime) {
        const auto lhsValue = lhs.toDateTime();
        const auto rhsValue = rhs.toDateTime();
        if (lhsValue == rhsValue)
            return 0;
        return lhsValue < rhsValue ? -1 : 1;
    } else if (lhs.typeId() == QMetaType::QStringList && rhs.typeId() == QMetaType::QStringList) {
        const auto lhsValue = lhs.toStringList();
        const auto rhsValue = rhs.toStringList();
        if (lhsValue == rhsValue)
            return 0;
        return lhsValue < rhsValue ? -1 : 1;
    } else if (lhs.canConvert<int>() && rhs.canConvert<int>()) {
        const auto lhsValue = lhs.toInt();
        const auto rhsValue = rhs.toInt();
        if (lhsValue == rhsValue)
            return 0;
        return lhsValue < rhsValue ? -1 : 1;
    } else if (lhs.canConvert<qreal>() && rhs.canConvert<qreal>()) {
        const auto lhsValue = lhs.toReal();
        const auto rhsValue = rhs.toReal();
        if (qFuzzyCompare(lhsValue, rhsValue))
            return 0;
        return lhsValue < rhsValue ? -1 : 1;
    }

    qWarning() << "Don't know how to compare" << lhs << "against" << rhs << "- returning 0";
    return 0;
}

}
