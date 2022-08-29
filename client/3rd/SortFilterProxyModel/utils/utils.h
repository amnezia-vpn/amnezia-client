#ifndef UTILS_H
#define UTILS_H

#include <QVariant>

namespace qqsfpm {

int compareVariants(const QVariant &lhs, const QVariant &rhs);

inline bool operator<(const QVariant &lhs, const QVariant &rhs) { return compareVariants(lhs, rhs) < 0; }
inline bool operator<=(const QVariant &lhs, const QVariant &rhs) { return compareVariants(lhs, rhs) <= 0; }
inline bool operator>(const QVariant &lhs, const QVariant &rhs) { return compareVariants(lhs, rhs) > 0; }
inline bool operator>=(const QVariant &lhs, const QVariant &rhs) { return compareVariants(lhs, rhs) >= 0; }

}

#endif // UTILS_H
