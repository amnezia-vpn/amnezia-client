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
    ServerUserNotInSudo,

    // Ssh connection errors
    SshRequsetDeniedError, SshInterruptedError, SshInternalError,

    // Ssh sftp errors
    SshSftpEofError, SshSftpNoSuchFileError, SshSftpPermissionDeniedError,
    SshSftpFailureError, SshSftpBadMessageError, SshSftpNoConnectionError,
    SshSftpConnectionLostError, SshSftpOpUnsupportedError, SshSftpInvalidHandleError,
    SshSftpNoSuchPathError, SshSftpFileAlreadyExistsError, SshSftpWriteProtectError,
    SshSftpNoMediaError,

    // Local errors
    FailedToSaveConfigData,
    OpenVpnConfigMissing,
    OpenVpnManagementServerError,
    ConfigMissing,

    // Distro errors
    OpenVpnExecutableMissing,
    ShadowSocksExecutableMissing,
    CloakExecutableMissing,
    AmneziaServiceConnectionFailed,
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
    CloakExecutableCrashed
};

} // namespace amnezia

Q_DECLARE_METATYPE(amnezia::ErrorCode)

#endif // DEFS_H
