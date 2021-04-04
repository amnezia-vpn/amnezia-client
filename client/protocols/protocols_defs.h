#ifndef PROTOCOLS_DEFS_H
#define PROTOCOLS_DEFS_H

#include <QObject>

namespace amnezia {
namespace protocols {
namespace openvpn {
static QString caCertPath() { return "/opt/amnezia/openvpn/pki/ca.crt"; }
static QString clientCertPath() { return "/opt/amnezia/openvpn/pki/issued"; }
static QString taKeyPath() { return "/opt/amnezia/openvpn/ta.key"; }
static QString clientsDirPath() { return "/opt/amnezia/openvpn/clients"; }
static QString openvpnDefaultPort() { return "1194"; }

}

namespace shadowsocks {
static int ssRemotePort() { return 6789; }
static int ssContainerPort() { return 8585; }
static QString ssEncryption() { return "chacha20-ietf-poly1305"; }
}

namespace cloak {
static QString ckPublicKeyPath() { return "/opt/amnezia/cloak/cloak_public.key"; }
static QString ckBypassUidKeyPath() { return "/opt/amnezia/cloak/cloak_bypass_uid.key"; }
static QString ckAdminKeyPath() { return "/opt/amnezia/cloak/cloak_admin_uid.key"; }
static QString ckDefaultPort() { return "443"; }
static QString ckDefaultRedirSite() { return "mail.ru"; }
}



} // namespace protocols
} // namespace amnezia

#endif // PROTOCOLS_DEFS_H
