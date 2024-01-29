#ifndef DEFS_H
#define DEFS_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia
{

    constexpr const qint16 qrMagicCode = 1984;

    struct ServerCredentials
    {
        QString hostName;
        QString userName;
        QString secretData;
        int port = 22;

        bool isValid() const
        {
            return !hostName.isEmpty() && !userName.isEmpty() && !secretData.isEmpty() && port > 0;
        }
    };

    enum ErrorCode {
        // General error codes
        NoError = 0,
        UnknownError = 100,
        InternalError = 101,
        NotImplementedError = 102,

        // Server errors
        ServerCheckFailed = 200,
        ServerPortAlreadyAllocatedError = 201,
        ServerContainerMissingError = 202,
        ServerDockerFailedError = 203,
        ServerCancelInstallation = 204,
        ServerUserNotInSudo = 205,
        ServerPacketManagerError = 206,

        // Ssh connection errors
        SshRequestDeniedError = 300,
        SshInterruptedError = 301,
        SshInternalError = 302,
        SshPrivateKeyError = 303,
        SshPrivateKeyFormatError = 304,
        SshTimeoutError = 305,

        // Ssh sftp errors
        SshSftpEofError = 400,
        SshSftpNoSuchFileError = 401,
        SshSftpPermissionDeniedError = 402,
        SshSftpFailureError = 403,
        SshSftpBadMessageError = 404,
        SshSftpNoConnectionError = 405,
        SshSftpConnectionLostError = 406,
        SshSftpOpUnsupportedError = 407,
        SshSftpInvalidHandleError = 408,
        SshSftpNoSuchPathError = 409,
        SshSftpFileAlreadyExistsError = 410,
        SshSftpWriteProtectError = 411,
        SshSftpNoMediaError = 412,

        // Local errors
        OpenVpnConfigMissing = 500,
        OpenVpnManagementServerError = 501,
        ConfigMissing = 502,

        // Distro errors
        OpenVpnExecutableMissing = 600,
        ShadowSocksExecutableMissing = 601,
        CloakExecutableMissing = 602,
        AmneziaServiceConnectionFailed = 603,
        ExecutableMissing = 604,

        // VPN errors
        OpenVpnAdaptersInUseError = 700,
        OpenVpnUnknownError = 701,
        OpenVpnTapAdapterError = 702,
        AddressPoolError = 703,

        // 3rd party utils errors
        OpenSslFailed = 800,
        ShadowSocksExecutableCrashed = 801,
        CloakExecutableCrashed = 802,

        // import and install errors
        ImportInvalidConfigError = 900,

        // Android errors
        AndroidError = 1000
    };

} // namespace amnezia

Q_DECLARE_METATYPE(amnezia::ErrorCode)

#endif // DEFS_H
