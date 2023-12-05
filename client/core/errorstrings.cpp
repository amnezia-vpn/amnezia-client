#include "errorstrings.h"

using namespace amnezia;

QString errorString(ErrorCode code){
    switch (code) {

    // General error codes
    case(NoError): return QObject::tr("No error");
    case(UnknownError): return QObject::tr("Unknown Error");
    case(NotImplementedError): return QObject::tr("Function not implemented");

    // Server errors
    case(ServerCheckFailed): return QObject::tr("Server check failed");
    case(ServerPortAlreadyAllocatedError): return QObject::tr("Server port already used. Check for another software");
    case(ServerContainerMissingError): return QObject::tr("Server error: Docker container missing");
    case(ServerDockerFailedError): return QObject::tr("Server error: Docker failed");
    case(ServerCancelInstallation): return QObject::tr("Installation canceled by user");
    case(ServerUserNotInSudo): return QObject::tr("The user does not have permission to use sudo");

    // Libssh errors
    case(SshRequestDeniedError): return QObject::tr("Ssh request was denied");
    case(SshInterruptedError): return QObject::tr("Ssh request was interrupted");
    case(SshInternalError): return QObject::tr("Ssh internal error");
    case(SshPrivateKeyError): return QObject::tr("Invalid private key or invalid passphrase entered");
    case(SshPrivateKeyFormatError): return QObject::tr("The selected private key format is not supported, use openssh ED25519 key types or PEM key types");
    case(SshTimeoutError): return QObject::tr("Timeout connecting to server");

    // Libssh sftp errors
    case(SshSftpEofError): return QObject::tr("Sftp error: End-of-file encountered");
    case(SshSftpNoSuchFileError): return QObject::tr("Sftp error: File does not exist");
    case(SshSftpPermissionDeniedError): return QObject::tr("Sftp error: Permission denied");
    case(SshSftpFailureError): return QObject::tr("Sftp error: Generic failure");
    case(SshSftpBadMessageError): return QObject::tr("Sftp error: Garbage received from server");
    case(SshSftpNoConnectionError): return QObject::tr("Sftp error: No connection has been set up");
    case(SshSftpConnectionLostError): return QObject::tr("Sftp error: There was a connection, but we lost it");
    case(SshSftpOpUnsupportedError): return QObject::tr("Sftp error: Operation not supported by libssh yet");
    case(SshSftpInvalidHandleError): return QObject::tr("Sftp error: Invalid file handle");
    case(SshSftpNoSuchPathError): return QObject::tr("Sftp error: No such file or directory path exists");
    case(SshSftpFileAlreadyExistsError): return QObject::tr("Sftp error: An attempt to create an already existing file or directory has been made");
    case(SshSftpWriteProtectError): return QObject::tr("Sftp error: Write-protected filesystem");
    case(SshSftpNoMediaError): return QObject::tr("Sftp error: No media was in remote drive");

    // Local errors
    case (OpenVpnConfigMissing): return QObject::tr("OpenVPN config missing");
    case (OpenVpnManagementServerError): return QObject::tr("OpenVPN management server error");

    // Distro errors
    case (OpenVpnExecutableMissing): return QObject::tr("OpenVPN executable missing");
    case (ShadowSocksExecutableMissing): return QObject::tr("ShadowSocks (ss-local) executable missing");
    case (CloakExecutableMissing): return QObject::tr("Cloak (ck-client) executable missing");
    case (AmneziaServiceConnectionFailed): return QObject::tr("Amnezia helper service error");
    case (OpenSslFailed): return QObject::tr("OpenSSL failed");

    // VPN errors
    case (OpenVpnAdaptersInUseError): return QObject::tr("Can't connect: another VPN connection is active");
    case (OpenVpnTapAdapterError): return QObject::tr("Can't setup OpenVPN TAP network adapter");
    case (AddressPoolError): return QObject::tr("VPN pool error: no available addresses");

    case (ImportInvalidConfigError): return QObject::tr("The config does not contain any containers and credentials for connecting to the server");

    case(InternalError):
    default:
        return QObject::tr("Internal error");
    }
}

QDebug operator<<(QDebug debug, const ErrorCode &e)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ErrorCode::" << int(e) << "(" << errorString(e) << ")";

    return debug;
}
