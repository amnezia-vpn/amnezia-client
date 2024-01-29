#include "errorstrings.h"

using namespace amnezia;

QString errorString(ErrorCode code) {
    QString errorMessage;

    switch (code) {

    // General error codes
    case(NoError): errorMessage = QObject::tr("No error");
    case(UnknownError): errorMessage = QObject::tr("Unknown Error");
    case(NotImplementedError): errorMessage = QObject::tr("Function not implemented");

    // Server errors
    case(ServerCheckFailed): errorMessage = QObject::tr("Server check failed");
    case(ServerPortAlreadyAllocatedError): errorMessage = QObject::tr("Server port already used. Check for another software");
    case(ServerContainerMissingError): errorMessage = QObject::tr("Server error: Docker container missing");
    case(ServerDockerFailedError): errorMessage = QObject::tr("Server error: Docker failed");
    case(ServerCancelInstallation): errorMessage = QObject::tr("Installation canceled by user");
    case(ServerUserNotInSudo): errorMessage = QObject::tr("The user does not have permission to use sudo");

    // Libssh errors
    case(SshRequestDeniedError): errorMessage = QObject::tr("Ssh request was denied");
    case(SshInterruptedError): errorMessage = QObject::tr("Ssh request was interrupted");
    case(SshInternalError): errorMessage = QObject::tr("Ssh internal error");
    case(SshPrivateKeyError): errorMessage = QObject::tr("Invalid private key or invalid passphrase entered");
    case(SshPrivateKeyFormatError): errorMessage = QObject::tr("The selected private key format is not supported, use openssh ED25519 key types or PEM key types");
    case(SshTimeoutError): errorMessage = QObject::tr("Timeout connecting to server");

    // Libssh sftp errors
    case(SshSftpEofError): errorMessage = QObject::tr("Sftp error: End-of-file encountered");
    case(SshSftpNoSuchFileError): errorMessage = QObject::tr("Sftp error: File does not exist");
    case(SshSftpPermissionDeniedError): errorMessage = QObject::tr("Sftp error: Permission denied");
    case(SshSftpFailureError): errorMessage = QObject::tr("Sftp error: Generic failure");
    case(SshSftpBadMessageError): errorMessage = QObject::tr("Sftp error: Garbage received from server");
    case(SshSftpNoConnectionError): errorMessage = QObject::tr("Sftp error: No connection has been set up");
    case(SshSftpConnectionLostError): errorMessage = QObject::tr("Sftp error: There was a connection, but we lost it");
    case(SshSftpOpUnsupportedError): errorMessage = QObject::tr("Sftp error: Operation not supported by libssh yet");
    case(SshSftpInvalidHandleError): errorMessage = QObject::tr("Sftp error: Invalid file handle");
    case(SshSftpNoSuchPathError): errorMessage = QObject::tr("Sftp error: No such file or directory path exists");
    case(SshSftpFileAlreadyExistsError): errorMessage = QObject::tr("Sftp error: An attempt to create an already existing file or directory has been made");
    case(SshSftpWriteProtectError): errorMessage = QObject::tr("Sftp error: Write-protected filesystem");
    case(SshSftpNoMediaError): errorMessage = QObject::tr("Sftp error: No media was in remote drive");

    // Local errors
    case (OpenVpnConfigMissing): errorMessage = QObject::tr("OpenVPN config missing");
    case (OpenVpnManagementServerError): errorMessage = QObject::tr("OpenVPN management server error");

    // Distro errors
    case (OpenVpnExecutableMissing): errorMessage = QObject::tr("OpenVPN executable missing");
    case (ShadowSocksExecutableMissing): errorMessage = QObject::tr("ShadowSocks (ss-local) executable missing");
    case (CloakExecutableMissing): errorMessage = QObject::tr("Cloak (ck-client) executable missing");
    case (AmneziaServiceConnectionFailed): errorMessage = QObject::tr("Amnezia helper service error");
    case (OpenSslFailed): errorMessage = QObject::tr("OpenSSL failed");

    // VPN errors
    case (OpenVpnAdaptersInUseError): errorMessage = QObject::tr("Can't connect: another VPN connection is active");
    case (OpenVpnTapAdapterError): errorMessage = QObject::tr("Can't setup OpenVPN TAP network adapter");
    case (AddressPoolError): errorMessage = QObject::tr("VPN pool error: no available addresses");

    case (ImportInvalidConfigError): errorMessage = QObject::tr("The config does not contain any containers and credentials for connecting to the server");

    // Android errors
    case (AndroidError): errorMessage = QObject::tr("VPN connection error");

    case(InternalError):
    default:
        errorMessage = QObject::tr("Internal error");
    }

    return QObject::tr("ErrorCode: %1. ").arg(code) + errorMessage;
}

QDebug operator<<(QDebug debug, const ErrorCode &e)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ErrorCode::" << int(e) << "(" << errorString(e) << ")";

    return debug;
}
