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
        UnknownError = 1,
        InternalError = 2,
        NotImplementedError = 3,

        // Server errors
        ServerCheckFailed = 4,
        ServerPortAlreadyAllocatedError = 5,
        ServerContainerMissingError = 6,
        ServerDockerFailedError = 7,
        ServerCancelInstallation = 8,
        ServerUserNotInSudo = 9,
        ServerPacketManagerError = 10,

        // Ssh connection errors
        SshRequestDeniedError = 11,
        SshInterruptedError = 12,
        SshInternalError = 13,
        SshPrivateKeyError = 14,
        SshPrivateKeyFormatError = 15,
        SshTimeoutError = 16,

        // Ssh sftp errors
        SshSftpEofError = 17,
        SshSftpNoSuchFileError = 18,
        SshSftpPermissionDeniedError = 19,
        SshSftpFailureError = 20,
        SshSftpBadMessageError = 21,
        SshSftpNoConnectionError = 22,
        SshSftpConnectionLostError = 23,
        SshSftpOpUnsupportedError = 24,
        SshSftpInvalidHandleError = 25,
        SshSftpNoSuchPathError = 26,
        SshSftpFileAlreadyExistsError = 27,
        SshSftpWriteProtectError = 28,
        SshSftpNoMediaError = 29,

        // Local errors
        OpenVpnConfigMissing = 30,
        OpenVpnManagementServerError = 31,
        ConfigMissing = 32,

        // Distro errors
        OpenVpnExecutableMissing = 33,
        ShadowSocksExecutableMissing = 34,
        CloakExecutableMissing = 35,
        AmneziaServiceConnectionFailed = 36,
        ExecutableMissing = 37,

        // VPN errors
        OpenVpnAdaptersInUseError = 38,
        OpenVpnUnknownError = 39,
        OpenVpnTapAdapterError = 40,
        AddressPoolError = 41,

        // 3rd party utils errors
        OpenSslFailed = 42,
        ShadowSocksExecutableCrashed = 43,
        CloakExecutableCrashed = 44,

        // import and install errors
        ImportInvalidConfigError = 45,

        // Android errors
        AndroidError = 46
    };

} // namespace amnezia

Q_DECLARE_METATYPE(amnezia::ErrorCode)

#endif // DEFS_H
