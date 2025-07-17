#include "errorstrings.h"

using namespace amnezia;

QString errorString(ErrorCode code) {
    QString errorMessage;

    switch (code) {

    // General error codes
    case(ErrorCode::NoError): errorMessage = QObject::tr("No error"); break;
    case(ErrorCode::UnknownError): errorMessage = QObject::tr("Unknown error"); break;
    case(ErrorCode::NotImplementedError): errorMessage = QObject::tr("Function not implemented"); break;
    case(ErrorCode::AmneziaServiceNotRunning): errorMessage = QObject::tr("Background service is not running"); break;
    case(ErrorCode::NotSupportedOnThisPlatform): errorMessage = QObject::tr("The selected protocol is not supported on the current platform"); break;

    // Server errors
    case(ErrorCode::ServerCheckFailed): errorMessage = QObject::tr("Server check failed"); break;
    case(ErrorCode::ServerPortAlreadyAllocatedError): errorMessage = QObject::tr("Server port already used. Check for another software"); break;
    case(ErrorCode::ServerContainerMissingError): errorMessage = QObject::tr("Server error: Docker container missing"); break;
    case(ErrorCode::ServerDockerFailedError): errorMessage = QObject::tr("Server error: Docker failed"); break;
    case(ErrorCode::ServerCancelInstallation): errorMessage = QObject::tr("Installation canceled by user"); break;
    case(ErrorCode::ServerUserNotInSudo): errorMessage = QObject::tr("The user is not a member of the sudo group"); break;
    case(ErrorCode::ServerPacketManagerError): errorMessage = QObject::tr("Server error: Package manager error"); break;
    case(ErrorCode::ServerSudoPackageIsNotPreinstalled): errorMessage = QObject::tr("The sudo package is not pre-installed on the server"); break;
    case(ErrorCode::ServerUserDirectoryNotAccessible): errorMessage = QObject::tr("The server user's home directory is not accessible"); break;
    case(ErrorCode::ServerUserNotAllowedInSudoers): errorMessage = QObject::tr("Action not allowed in sudoers"); break;
    case(ErrorCode::ServerUserPasswordRequired): errorMessage = QObject::tr("The user's password is required"); break;
    case(ErrorCode::ServerDockerOnCgroupsV2): errorMessage = QObject::tr("Docker error: runc doesn't work on cgroups v2"); break;
    case(ErrorCode::ServerCgroupMountpoint): errorMessage = QObject::tr("Server error: cgroup mountpoint does not exist"); break;
    case(ErrorCode::DockerPullRateLimit): errorMessage = QObject::tr("Docker error: The pull rate limit has been reached"); break;

    // Libssh errors
    case(ErrorCode::SshRequestDeniedError): errorMessage = QObject::tr("SSH request was denied"); break;
    case(ErrorCode::SshInterruptedError): errorMessage = QObject::tr("SSH request was interrupted"); break;
    case(ErrorCode::SshInternalError): errorMessage = QObject::tr("SSH internal error"); break;
    case(ErrorCode::SshPrivateKeyError): errorMessage = QObject::tr("Invalid private key or invalid passphrase entered"); break;
    case(ErrorCode::SshPrivateKeyFormatError): errorMessage = QObject::tr("The selected private key format is not supported, use openssh ED25519 key types or PEM key types"); break;
    case(ErrorCode::SshTimeoutError): errorMessage = QObject::tr("Timeout connecting to server"); break;

    // Ssh scp errors
    case(ErrorCode::SshScpFailureError): errorMessage = QObject::tr("SCP error: Generic failure"); break;

    // Local errors
    case (ErrorCode::OpenVpnConfigMissing): errorMessage = QObject::tr("OpenVPN config missing"); break;
    case (ErrorCode::OpenVpnManagementServerError): errorMessage = QObject::tr("OpenVPN management server error"); break;

    // Distro errors
    case (ErrorCode::OpenVpnExecutableMissing): errorMessage = QObject::tr("OpenVPN executable missing"); break;
    case (ErrorCode::ShadowSocksExecutableMissing): errorMessage = QObject::tr("Shadowsocks (ss-local) executable missing"); break;
    case (ErrorCode::CloakExecutableMissing): errorMessage = QObject::tr("Cloak (ck-client) executable missing"); break;
    case (ErrorCode::AmneziaServiceConnectionFailed): errorMessage = QObject::tr("Amnezia helper service error"); break;
    case (ErrorCode::OpenSslFailed): errorMessage = QObject::tr("OpenSSL failed"); break;

    // VPN errors
    case (ErrorCode::OpenVpnAdaptersInUseError): errorMessage = QObject::tr("Can't connect: another VPN connection is active"); break;
    case (ErrorCode::OpenVpnTapAdapterError): errorMessage = QObject::tr("Can't setup OpenVPN TAP network adapter"); break;
    case (ErrorCode::AddressPoolError): errorMessage = QObject::tr("VPN pool error: no available addresses"); break;

    case (ErrorCode::ImportInvalidConfigError): errorMessage = QObject::tr("The config does not contain any containers and credentials for connecting to the server"); break;
    case (ErrorCode::ImportOpenConfigError): errorMessage = QObject::tr("Unable to open config file"); break;
    case(ErrorCode::NoInstalledContainersError): errorMessage = QObject::tr("VPN Protocols is not installed.\n Please install VPN container at first"); break;

    // Android errors
    case (ErrorCode::AndroidError): errorMessage = QObject::tr("VPN connection error"); break;

    // Api errors
    case (ErrorCode::ApiConfigDownloadError): errorMessage = QObject::tr("Error when retrieving configuration from API"); break;
    case (ErrorCode::ApiConfigAlreadyAdded): errorMessage = QObject::tr("This config has already been added to the application"); break;
    case (ErrorCode::ApiConfigEmptyError): errorMessage = QObject::tr("In the response from the server, an empty config was received"); break;
    case (ErrorCode::ApiConfigSslError): errorMessage = QObject::tr("SSL error occurred"); break;
    case (ErrorCode::ApiConfigTimeoutError): errorMessage = QObject::tr("Server response timeout on api request"); break;
    case (ErrorCode::ApiMissingAgwPublicKey): errorMessage = QObject::tr("Missing AGW public key"); break;
    case (ErrorCode::ApiConfigDecryptionError): errorMessage = QObject::tr("Failed to decrypt response payload"); break;
    case (ErrorCode::ApiServicesMissingError): errorMessage = QObject::tr("Missing list of available services"); break;
    case (ErrorCode::ApiConfigLimitError): errorMessage = QObject::tr("The limit of allowed configurations per subscription has been exceeded"); break;
    case (ErrorCode::ApiNotFoundError): errorMessage = QObject::tr("Error when retrieving configuration from API"); break;
    case (ErrorCode::ApiMigrationError): errorMessage = QObject::tr("A migration error has occurred. Please contact our technical support"); break;
    case (ErrorCode::ApiUpdateRequestError): errorMessage = QObject::tr("Please update the application to use this feature"); break;

    // QFile errors
    case(ErrorCode::OpenError): errorMessage = QObject::tr("QFile error: The file could not be opened"); break;
    case(ErrorCode::ReadError): errorMessage = QObject::tr("QFile error: An error occurred when reading from the file"); break;
    case(ErrorCode::PermissionsError): errorMessage = QObject::tr("QFile error: The file could not be accessed"); break;
    case(ErrorCode::UnspecifiedError): errorMessage =  QObject::tr("QFile error: An unspecified error occurred"); break;
    case(ErrorCode::FatalError): errorMessage =  QObject::tr("QFile error: A fatal error occurred"); break;
    case(ErrorCode::AbortError): errorMessage =  QObject::tr("QFile error: The operation was aborted"); break;

    case(ErrorCode::InternalError):
    default:
        errorMessage = QObject::tr("Internal error"); break;
    }

    return QObject::tr("ErrorCode: %1. ").arg(code) + errorMessage;
}

QDebug operator<<(QDebug debug, const ErrorCode &e)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ErrorCode::" << int(e) << "(" << errorString(e) << ")";

    return debug;
}
