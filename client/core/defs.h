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

        // Ssh scp errors
        SshScpFailureError = 400,

        // Local errors
        OpenVpnConfigMissing = 500,
        OpenVpnManagementServerError = 501,

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
        AndroidError = 1000,

        // Api errors
        ApiConfigDownloadError = 1100,
        ApiConfigAlreadyAdded = 1101,

        // QFile errors
        OpenError = 1200,
        ReadError = 1201,
        PermissionsError = 1202,
        UnspecifiedError = 1203,
    };

} // namespace amnezia

Q_DECLARE_METATYPE(amnezia::ErrorCode)

#endif // DEFS_H
