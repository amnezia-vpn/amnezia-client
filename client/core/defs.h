#ifndef DEFS_H
#define DEFS_H

#include <QObject>

namespace amnezia {

enum class Protocol {
    Any,
    OpenVpn,
    ShadowSocks,
    OpenVpnOverCloak,
    WireGuard
};

enum class DockerContainer {
    None,
    OpenVpn,
    ShadowSocks,
    OpenVpnOverCloak,
    WireGuard
};

static DockerContainer containerForProto(Protocol proto)
{
    Q_ASSERT(proto != Protocol::Any);

    switch (proto) {
    case Protocol::OpenVpn: return DockerContainer::OpenVpn;
    case Protocol::OpenVpnOverCloak: return DockerContainer::OpenVpnOverCloak;
    case Protocol::ShadowSocks: return DockerContainer::ShadowSocks;
    case Protocol::WireGuard: return DockerContainer::WireGuard;
    case Protocol::Any: return DockerContainer::None;
    }
}

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
static QString key_openvpn_config_data() { return "openvpn_config_data"; }
static QString key_openvpn_config_path() { return "openvpn_config_path"; }
static QString key_shadowsocks_config_data() { return "shadowsocks_config_data"; }
static QString key_cloak_config_data() { return "cloak_config_data"; }

}

} // namespace amnezia

#endif // DEFS_H
