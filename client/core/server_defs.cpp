#include "server_defs.h"

//QString fblink::containerToString(fblink::DockerContainer container)
//{
//    switch (container) {
//    case(DockerContainer::OpenVpn): return "fblink-openvpn";
//    case(DockerContainer::OpenVpnOverCloak): return "fblink-openvpn-cloak";
//    case(DockerContainer::OpenVpnOverShadowSocks): return "fblink-shadowsocks";
//    default: return "";
//    }
//}

QString fblink::server::getDockerfileFolder(fblink::DockerContainer container)
{
    return "/opt/fblink/" + ContainerProps::containerToString(container);
}
