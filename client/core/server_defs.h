#ifndef SERVER_DEFS_H
#define SERVER_DEFS_H

#include <QObject>
#include "protocols/protocols_defs.h"

namespace amnezia {
namespace server {
//QString getContainerName(amnezia::DockerContainer container);
QString getDockerfileFolder(amnezia::DockerContainer container);

}
}

#endif // SERVER_DEFS_H
