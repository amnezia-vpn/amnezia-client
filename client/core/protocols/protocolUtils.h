#ifndef PROTOCOLUTILS_H
#define PROTOCOLUTILS_H

#include <QList>
#include <QMap>
#include <QString>
#include <QJsonObject>

#include "core/utils/protocolEnum.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

namespace amnezia
{
    namespace ProtocolUtils
    {
        QList<Proto> allProtocols();

        // spelling may differ for various protocols - TCP for OpenVPN, tcp for others
        TransportProto transportProtoFromString(QString p);
        QString transportProtoToString(TransportProto proto, Proto p = Proto::Unknown);

        Proto protoFromString(QString p);
        QString protoToString(Proto p);

        QMap<Proto, QString> protocolHumanNames();
        QMap<Proto, QString> protocolDescriptions();

        ServiceType protocolService(Proto p);

        int getPortForInstall(Proto p);

        int defaultPort(Proto p);
        bool defaultPortChangeable(Proto p);

        TransportProto defaultTransportProto(Proto p);
        bool defaultTransportProtoChangeable(Proto p);

        QString key_proto_config_data(Proto p);
        QString key_proto_config_path(Proto p);

        QString getProtocolVersion(const QJsonObject &protocolConfig);
        QString getProtocolVersionString(const QJsonObject &protocolConfig);

        QStringList parseAllowedIps(const QString &allowedIpsString);
        QStringList normalizeAllowedIpsList(const QStringList &allowedIps);
        bool isFullTunnelAllowedIps(const QStringList &allowedIps);
        bool hasServerSideSplitTunneling(const QStringList &allowedIps, const QString &nativeConfig = QString());
    }
}

#endif // PROTOCOLUTILS_H


