#ifndef DEFS_H
#define DEFS_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia {

//static DockerContainer containerForProto(Protocol proto)
//{
//    Q_ASSERT(proto != Protocol::Any);

//    switch (proto) {
//    case Protocol::OpenVpn: return DockerContainer::OpenVpn;
//    case Protocol::OpenVpnOverCloak: return DockerContainer::OpenVpnOverCloak;
//    case Protocol::ShadowSocksOverOpenVpn: return DockerContainer::ShadowSocksOverOpenVpn;
//    case Protocol::WireGuard: return DockerContainer::WireGuard;
//    case Protocol::Any: return DockerContainer::None;
//    }
//}

struct ServerCredentials
{
    QString hostName;
    QString userName;
    QString password;
    int port = 22;

    bool isValid() { return !hostName.isEmpty() && !userName.isEmpty() && !password.isEmpty() && port > 0; }
};

enum ErrorCode
{
    // General error codes
    NoError = 0,
    UnknownError,
    InternalError,
    NotImplementedError,

    // Server errors
    ServerCheckFailed,
    ServerPortAlreadyAllocatedError,
    ServerContainerMissingError,

    // Ssh connection errors
    SshSocketError, SshTimeoutError, SshProtocolError,
    SshHostKeyError, SshKeyFileError, SshAuthenticationError,
    SshClosedByServerError, SshInternalError,

    // Ssh remote process errors
    SshRemoteProcessCreationError,
    FailedToStartRemoteProcessError, RemoteProcessCrashError,
    SshSftpError,

    // Local errors
    FailedToSaveConfigData,
    OpenVpnConfigMissing,
    OpenVpnManagementServerError,
    EasyRsaError,

    // Distro errors
    OpenVpnExecutableMissing,
    EasyRsaExecutableMissing,
    ShadowSocksExecutableMissing,
    CloakExecutableMissing,
    AmneziaServiceConnectionFailed,

    // VPN errors
    OpenVpnAdaptersInUseError,
    OpenVpnUnknownError,

    // 3rd party utils errors
    OpenVpnExecutableCrashed,
    ShadowSocksExecutableCrashed,
    CloakExecutableCrashed
};

namespace config {
// config keys
const char key_openvpn_config_data[] = "openvpn_config_data";
const char key_openvpn_config_path[] = "openvpn_config_path";
const char key_shadowsocks_config_data[] = "shadowsocks_config_data";
const char key_cloak_config_data[] = "cloak_config_data";

}

} // namespace amnezia

#endif // DEFS_H
