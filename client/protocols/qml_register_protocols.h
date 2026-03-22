#ifndef QML_REGISTER_PROTOCOLS_H
#define QML_REGISTER_PROTOCOLS_H

#include "protocols_defs.h"

#include <QObject>
#include <QDebug>
#include <QQmlEngine>

namespace fblink {

using namespace fblink::ProtocolEnumNS;

void declareQmlProtocolEnum() {
    qmlRegisterUncreatableMetaObject(
        ProtocolEnumNS::staticMetaObject,
        "ProtocolEnum",
        1, 0,
        "ProtocolEnum",
        "Error: only enums"
        );

    qmlRegisterUncreatableMetaObject(
        ProtocolEnumNS::staticMetaObject,
        "ProtocolEnum",
        1, 0,
        "TransportProto",
        "Error: only enums"
        );

    qmlRegisterUncreatableMetaObject(
        ProtocolEnumNS::staticMetaObject,
        "ProtocolEnum",
        1, 0,
        "ServiceType",
        "Error: only enums"
        );
}

} // namespace fblink

QDebug operator<<(QDebug debug, const fblink::Proto &p);

#endif // QML_REGISTER_PROTOCOLS_H
