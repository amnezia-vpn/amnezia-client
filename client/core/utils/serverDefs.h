#ifndef SERVERDEFS_H
#define SERVERDEFS_H

#include <QObject>
#include "containers/containers_defs.h"

namespace amnezia {
namespace server {
//QString getContainerName(amnezia::DockerContainer container);
QString getDockerfileFolder(amnezia::DockerContainer container);

}
}

#endif // SERVERDEFS_H
