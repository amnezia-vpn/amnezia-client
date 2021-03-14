#ifndef DEFS_H
#define DEFS_H

#include <QObject>

namespace amnezia {

enum class Protocol {
    Any,
    OpenVpn,
    ShadowSocks,
    WireGuard
};

enum class DockerContainer {
    OpenVpn,
    ShadowSocks,
    WireGuard
};

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

    // Local errors
    FailedToSaveConfigData,
    OpenVpnConfigMissing,
    OpenVpnManagementServerError,
    EasyRsaError,

    // Distro errors
    OpenVpnExecutableMissing,
    EasyRsaExecutableMissing,
    ShadowSocksExecutableMissing,
    AmneziaServiceConnectionFailed,

    // VPN errors
    OpenVpnAdaptersInUseError,
    OpenVpnUnknownError
};

namespace config {
// config keys
static QString key_openvpn_config_data() { return "openvpn_config_data"; }
static QString key_openvpn_config_path() { return "openvpn_config_path"; }
static QString key_shadowsocks_config_data() { return "shadowsocks_config_data"; }

}


} // namespace amnezia

#endif // DEFS_H
