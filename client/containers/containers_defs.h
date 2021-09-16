#ifndef CONTAIERNS_DEFS_H
#define CONTAIERNS_DEFS_H

#include <QObject>
#include <QQmlEngine>

#include "../protocols/protocols_defs.h"

using namespace amnezia;

namespace amnezia {

namespace ContainerEnumNS {
Q_NAMESPACE
enum class DockerContainer {
    None,
    OpenVpn,
    OpenVpnOverShadowSocks,
    OpenVpnOverCloak,
    WireGuard
};
Q_ENUM_NS(DockerContainer)
} // namespace ContainerEnumNS

using namespace ContainerEnumNS;
using namespace ProtocolEnumNS;

DockerContainer containerFromString(const QString &container);
QString containerToString(DockerContainer container);

QVector<DockerContainer> allContainers();

QMap<DockerContainer, QString> containerHumanNames();
QMap<DockerContainer, QString> containerDescriptions();
bool isContainerVpnType(DockerContainer c);

QVector<Protocol> protocolsForContainer(DockerContainer container);

static void declareQmlContainerEnum() {
    qmlRegisterUncreatableMetaObject(
                ContainerEnumNS::staticMetaObject,
                "ContainerEnum",
                1, 0,
                "ContainerEnum",
                "Error: only enums"
                );
}

} // namespace amnezia

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c);

#endif // CONTAIERNS_DEFS_H
