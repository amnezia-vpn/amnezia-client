#ifndef VPNPROTOCOL_H
#define VPNPROTOCOL_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QtQml/qqml.h>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"

using namespace amnezia;

//todo change name
namespace Vpn
{
    Q_NAMESPACE
    enum ConnectionState {
        Unknown,
        Disconnected,
        Preparing,
        Connecting,
        Connected,
        Disconnecting,
        Reconnecting,
        Error
    };
    Q_ENUM_NS(ConnectionState)

    static void declareQmlVpnConnectionStateEnum() {
        qmlRegisterUncreatableMetaObject(
            Vpn::staticMetaObject,
            "ConnectionState",
            1, 0,
            "ConnectionState",
            "Error: only enums"
            );
    }
}

#endif // VPNPROTOCOL_H
