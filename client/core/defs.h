#ifndef DEFS_H
#define DEFS_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia {

constexpr const qint16 qrMagicCode = 1984;

struct ServerCredentials
{
    QString hostName;
    QString userName;
    QString password;
    int port = 22;

    bool isValid() const { return !hostName.isEmpty() && !userName.isEmpty() && !password.isEmpty() && port > 0; }
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
    ServerDockerFailedError,
    ServerCancelInstallation,

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
    ConfigMissing,
    V2RayTrojanPasswordMissing,
    

    // Distro errors
    OpenVpnExecutableMissing,
    ShadowSocksExecutableMissing,
    CloakExecutableMissing,
    AmneziaServiceConnectionFailed,
    V2RayExecutableMissing,
    ExecutableMissing,

    // VPN errors
    OpenVpnAdaptersInUseError,
    OpenVpnUnknownError,
    OpenVpnTapAdapterError,
    AddressPoolError,

    // 3rd party utils errors
    OpenSslFailed,
    OpenVpnExecutableCrashed,
    ShadowSocksExecutableCrashed,
    CloakExecutableCrashed,
    V2RayExecutableCrashed,
    V2RayKeyMissing
};

} // namespace amnezia

Q_DECLARE_METATYPE(amnezia::ErrorCode)

#endif // DEFS_H
