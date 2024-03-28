#include "errorstrings.h"

using namespace amnezia;

QString errorString(ErrorCode code) {
    QString errorMessage;

    switch (code) {

    // General error codes
    case(NoError): errorMessage = QObject::tr("No error"); break;
    case(UnknownError): errorMessage = QObject::tr("Unknown Error"); break;
    case(NotImplementedError): errorMessage = QObject::tr("Function not implemented"); break;

    // Server errors
    case(ServerCheckFailed): errorMessage = QObject::tr("Server check failed"); break;
    case(ServerPortAlreadyAllocatedError): errorMessage = QObject::tr("Server port already used. Check for another software"); break;
    case(ServerContainerMissingError): errorMessage = QObject::tr("Server error: Docker container missing"); break;
    case(ServerDockerFailedError): errorMessage = QObject::tr("Server error: Docker failed"); break;
    case(ServerCancelInstallation): errorMessage = QObject::tr("Installation canceled by user"); break;
    case(ServerUserNotInSudo): errorMessage = QObject::tr("The user does not have permission to use sudo"); break;
    case(ServerDockerVersionError): errorMessage = QObject::tr("Docker version is not compatible. Try installing Docker Engine manually."); break;

    // Libssh errors
    case(SshRequestDeniedError): errorMessage = QObject::tr("Ssh request was denied"); break;
    case(SshInterruptedError): errorMessage = QObject::tr("Ssh request was interrupted"); break;
    case(SshInternalError): errorMessage = QObject::tr("Ssh internal error"); break;
    case(SshPrivateKeyError): errorMessage = QObject::tr("Invalid private key or invalid passphrase entered"); break;
    case(SshPrivateKeyFormatError): errorMessage = QObject::tr("The selected private key format is not supported, use openssh ED25519 key types or PEM key types"); break;
    case(SshTimeoutError): errorMessage = QObject::tr("Timeout connecting to server"); break;

    // Ssh scp errors
    case(SshScpFailureError): errorMessage = QObject::tr("Scp error: Generic failure"); break;

    // Local errors
    case (OpenVpnConfigMissing): errorMessage = QObject::tr("OpenVPN config missing"); break;
    case (OpenVpnManagementServerError): errorMessage = QObject::tr("OpenVPN management server error"); break;

    // Distro errors
    case (OpenVpnExecutableMissing): errorMessage = QObject::tr("OpenVPN executable missing"); break;
    case (ShadowSocksExecutableMissing): errorMessage = QObject::tr("ShadowSocks (ss-local) executable missing"); break;
    case (CloakExecutableMissing): errorMessage = QObject::tr("Cloak (ck-client) executable missing"); break;
    case (AmneziaServiceConnectionFailed): errorMessage = QObject::tr("Amnezia helper service error"); break;
    case (OpenSslFailed): errorMessage = QObject::tr("OpenSSL failed"); break;

    // VPN errors
    case (OpenVpnAdaptersInUseError): errorMessage = QObject::tr("Can't connect: another VPN connection is active"); break;
    case (OpenVpnTapAdapterError): errorMessage = QObject::tr("Can't setup OpenVPN TAP network adapter"); break;
    case (AddressPoolError): errorMessage = QObject::tr("VPN pool error: no available addresses"); break;

    case (ImportInvalidConfigError): errorMessage = QObject::tr("The config does not contain any containers and credentials for connecting to the server"); break;

    // Android errors
    case (AndroidError): errorMessage = QObject::tr("VPN connection error"); break;

    // Api errors
    case (ApiConfigDownloadError): errorMessage = QObject::tr("Error when retrieving configuration from API"); break;
    case (ApiConfigAlreadyAdded): errorMessage = QObject::tr("This config has already been added to the application"); break;

    // QFile errors
    case(OpenError): errorMessage = QObject::tr("QFile error: The file could not be opened"); break;
    case(ReadError): errorMessage = QObject::tr("QFile error: An error occurred when reading from the file"); break;
    case(PermissionsError): errorMessage = QObject::tr("QFile error: The file could not be accessed"); break;
    case(UnspecifiedError): errorMessage =  QObject::tr("QFile error: An unspecified error occurred"); break;
    case(FatalError): errorMessage =  QObject::tr("QFile error: A fatal error occurred"); break;
    case(AbortError): errorMessage =  QObject::tr("QFile error: The operation was aborted"); break;

    case(InternalError):
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
