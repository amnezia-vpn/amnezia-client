#ifndef QML_REGISTER_PROTOCOLS_H
#define QML_REGISTER_PROTOCOLS_H

#include "core/utils/protocolEnum.h"

#include <QObject>
#include <QDebug>
#include <QQmlEngine>

namespace amnezia {

using namespace amnezia::ProtocolEnumNS;

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

} // namespace amnezia

#endif // QML_REGISTER_PROTOCOLS_H
