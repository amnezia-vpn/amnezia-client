#ifndef SERVER_DEFS_H
#define SERVER_DEFS_H

#include <QObject>
#include "core/defs.h"

namespace amnezia {
namespace server {
QString getContainerName(amnezia::DockerContainer container);
QString getDockerfileFolder(amnezia::DockerContainer container);

}
}

#endif // SERVER_DEFS_H
