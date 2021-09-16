#ifndef CONTAIERNS_DEFS_H
#define CONTAIERNS_DEFS_H

#include <QObject>

#include "../protocols/protocols_defs.h"

using namespace amnezia;

namespace amnezia {
Q_NAMESPACE

enum class DockerContainer {
    None,
    OpenVpn,
    OpenVpnOverShadowSocks,
    OpenVpnOverCloak,
    WireGuard
};
Q_ENUM_NS(DockerContainer)

DockerContainer containerFromString(const QString &container);
QString containerToString(DockerContainer container);

QVector<DockerContainer> allContainers();

QMap<DockerContainer, QString> containerHumanNames();
QMap<DockerContainer, QString> containerDescriptions();
bool isContainerVpnType(DockerContainer c);

QVector<Protocol> protocolsForContainer(DockerContainer container);

} // namespace amnezia

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c);

#endif // CONTAIERNS_DEFS_H
