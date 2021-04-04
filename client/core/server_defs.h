#ifndef SERVER_DEFS_H
#define SERVER_DEFS_H

#include <QObject>
#include "core/defs.h"

namespace amnezia {
namespace server {
QString getContainerName(amnezia::DockerContainer container);
QString getDockerfileFolder(amnezia::DockerContainer container);

static QString vpnDefaultSubnetIp() { return "10.8.0.0"; }
static QString vpnDefaultSubnetMask() { return "255.255.255.0"; }
static QString vpnDefaultSubnetMaskVal() { return "24"; }

}
}

#endif // SERVER_DEFS_H
