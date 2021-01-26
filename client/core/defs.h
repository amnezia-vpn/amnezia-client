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
};

enum ErrorCode
{
    // General error codes
    NoError = 0,
    UnknownError,
    InternalError,
    NotImplementedError,

    // Server errorz
    ServerCheckFailed,

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
    AmneziaServiceConnectionFailed,

    // VPN errors
    OpenVpnAdaptersInUseError,
    OpenVpnUnknownError
};

} // namespace amnezia

#endif // DEFS_H
