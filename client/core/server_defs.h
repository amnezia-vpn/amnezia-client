#ifndef SERVER_DEFS_H
#define SERVER_DEFS_H

#include <QObject>
#include "containers/containers_defs.h"

namespace fblink {
namespace server {
//QString getContainerName(fblink::DockerContainer container);
QString getDockerfileFolder(fblink::DockerContainer container);

}
}

#endif // SERVER_DEFS_H
