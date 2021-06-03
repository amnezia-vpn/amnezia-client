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

    // Ssh connection errors
    case(SshSocketError): return QObject::tr("Ssh connection error");
    case(SshTimeoutError): return QObject::tr("Ssh connection timeout");
    case(SshProtocolError): return QObject::tr("Ssh protocol error");
    case(SshHostKeyError): return QObject::tr("Ssh server ket check failed");
    case(SshKeyFileError): return QObject::tr("Ssh key file error");
    case(SshAuthenticationError): return QObject::tr("Ssh authentication error");
    case(SshClosedByServerError): return QObject::tr("Ssh session closed");
    case(SshInternalError): return QObject::tr("Ssh internal error");

    // Ssh remote process errors
    case(SshRemoteProcessCreationError): return QObject::tr("Failed to create remote process on server");
    case(FailedToStartRemoteProcessError): return QObject::tr("Failed to start remote process on server");
    case(RemoteProcessCrashError): return QObject::tr("Remote process on server crashed");

    // Local errors
    case (FailedToSaveConfigData): return QObject::tr("Failed to save config to disk");
    case (OpenVpnConfigMissing): return QObject::tr("OpenVPN config missing");
    case (OpenVpnManagementServerError): return QObject::tr("OpenVPN management server error");
    case (EasyRsaError): return QObject::tr("EasyRSA runtime error");

    // Distro errors
    case (OpenVpnExecutableMissing): return QObject::tr("OpenVPN executable missing");
    case (EasyRsaExecutableMissing): return QObject::tr("EasyRsa executable missing");
    case (AmneziaServiceConnectionFailed): return QObject::tr("Amnezia helper service error");

    // VPN errors
    case (OpenVpnAdaptersInUseError): return QObject::tr("Can't connect: another VPN connection is active");
    case (OpenVpnTapAdapterError): return QObject::tr("Can't setup OpenVPN TAP network adapter");

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
