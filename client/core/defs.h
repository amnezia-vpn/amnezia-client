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

    struct InstalledAppInfo {
        QString appName;
        QString packageName;
        QString appPath;

        bool operator==(const InstalledAppInfo& other) const {
            if (!packageName.isEmpty()) {
                return packageName == other.packageName;
            } else {
                return appPath == other.appPath;
            }
        }
    };

    namespace error_code_ns
    {
      Q_NAMESPACE
      // TODO: change to enum class
      enum ErrorCode {
        // General error codes
        NoError = 0,
        UnknownError = 100,
        InternalError = 101,
        NotImplementedError = 102,
        AmneziaServiceNotRunning = 103,

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
        XrayExecutableMissing = 605,
        Tun2SockExecutableMissing = 606,

        // VPN errors
        OpenVpnAdaptersInUseError = 700,
        OpenVpnUnknownError = 701,
        OpenVpnTapAdapterError = 702,
        AddressPoolError = 703,

        // 3rd party utils errors
        OpenSslFailed = 800,
        ShadowSocksExecutableCrashed = 801,
        CloakExecutableCrashed = 802,
        XrayExecutableCrashed = 803,
        Tun2SockExecutableCrashed = 804,

        // import and install errors
        ImportInvalidConfigError = 900,
        ImportOpenConfigError = 901,

        // Android errors
        AndroidError = 1000,

        // Api errors
        ApiConfigDownloadError = 1100,
        ApiConfigAlreadyAdded = 1101,
        ApiConfigEmptyError = 1102,
        ApiConfigTimeoutError = 1103,
        ApiConfigSslError = 1104,
        ApiMissingAgwPublicKey = 1105,
        ApiConfigDecryptionError = 1106,

        // QFile errors
        OpenError = 1200,
        ReadError = 1201,
        PermissionsError = 1202,
        UnspecifiedError = 1203,
        FatalError = 1204,
        AbortError = 1205
      };
      Q_ENUM_NS(ErrorCode)
    }

    using ErrorCode = error_code_ns::ErrorCode;

} // namespace amnezia

Q_DECLARE_METATYPE(amnezia::ErrorCode)

#endif // DEFS_H
