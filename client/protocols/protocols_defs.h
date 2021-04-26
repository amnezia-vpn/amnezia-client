#ifndef PROTOCOLS_DEFS_H
#define PROTOCOLS_DEFS_H

#include <QObject>

namespace amnezia {

inline QString nonEmpty(const QString &val, const QString &deflt) { return val.isEmpty() ? deflt : val; }


namespace config_key {

// Json config strings
const char hostName[] = "hostName";
const char userName[] = "userName";
const char password[] = "password";
const char port[] = "port";
const char description[] = "description";


const char containers[] = "containers";
const char container[] = "container";
const char defaultContainer[] = "defaultContainer";

const char protocols[] = "protocols";
const char protocol[] = "protocol";

const char transport_protocol[] = "transport_protocol";
const char cipher[] = "cipher";
const char hash[] = "hash";

const char site[] = "site";

const char subnet_address[] = "subnet_address";
const char subnet_mask[] = "subnet_mask";
const char subnet_mask_val[] = "subnet_mask_val";

const char openvpn[] = "openvpn";
const char shadowsocks[] = "shadowsocks";
const char cloak[] = "cloak";

}

namespace protocols {

const char vpnDefaultSubnetAddress[] = "10.8.0.0";
const char vpnDefaultSubnetMask[] = "255.255.255.0";
const char vpnDefaultSubnetMaskVal[] = "24";

namespace openvpn {
const char caCertPath[] = "/opt/amnezia/openvpn/pki/ca.crt";
const char clientCertPath[] = "/opt/amnezia/openvpn/pki/issued";
const char taKeyPath[] = "/opt/amnezia/openvpn/ta.key";
const char clientsDirPath[] = "/opt/amnezia/openvpn/clients";
const char openvpnDefaultPort[] = "1194";
const char openvpnDefaultProto[] = "UDP";
const char openvpnDefaultCipher[] = "AES-256-GCM";
const char openvpnDefaultHash[] = "SHA512";
const bool blockOutsideDNS = true;
}

namespace shadowsocks {
const char ssDefaultPort[] = "6789";
const char ssLocalProxyPort[] = "8585";
const char ssDefaultCipher[] = "chacha20-ietf-poly1305";
}

namespace cloak {
const char ckPublicKeyPath[] = "/opt/amnezia/cloak/cloak_public.key";
const char ckBypassUidKeyPath[] = "/opt/amnezia/cloak/cloak_bypass_uid.key";
const char ckAdminKeyPath[] = "/opt/amnezia/cloak/cloak_admin_uid.key";
const char ckDefaultPort[] = "443";
const char ckDefaultRedirSite[] = "mail.ru";
}



} // namespace protocols
} // namespace amnezia

#endif // PROTOCOLS_DEFS_H
