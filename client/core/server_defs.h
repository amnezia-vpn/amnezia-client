#ifndef SERVER_DEFS_H
#define SERVER_DEFS_H

#include <QObject>
#include "core/defs.h"

namespace amnezia {
namespace server {
QString getContainerName(amnezia::DockerContainer container);
QString getDockerfileFolder(amnezia::DockerContainer container);

const char vpnDefaultSubnetIp[] = "10.8.0.0";
const char vpnDefaultSubnetMask[] = "255.255.255.0";
const char vpnDefaultSubnetMaskVal[] = "24";

}
}

#endif // SERVER_DEFS_H
