#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

#include <QThread>

#include <chrono>

#include "logger.h"
#include "ikev2_vpn_protocol_windows.h"
#include "utilities.h"

static Ikev2Protocol* self = nullptr;
static std::mutex rasDialFuncMutex;

extern "C" {
static void WINAPI RasDialFuncCallback(UINT unMsg,
                                       RASCONNSTATE rasconnstate,
                                       DWORD dwError );
}

Ikev2Protocol::Ikev2Protocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    self = this;
    readIkev2Configuration(configuration);
}

Ikev2Protocol::~Ikev2Protocol()
{
    qDebug() << "IpsecProtocol::~IpsecProtocol()";
    disconnect_vpn();
    Ikev2Protocol::stop();
}

void Ikev2Protocol::stop()
{
    setConnectionState(VpnProtocol::Disconnecting);
    {
        if (! disconnect_vpn() ){
            qDebug()<<"We don't disconnect";
            setConnectionState(VpnProtocol::Error);
        }
        else {
            setConnectionState(VpnProtocol::Disconnected);
        }
    }
}

void Ikev2Protocol::newConnectionStateEventReceived(UINT unMsg, tagRASCONNSTATE rasconnstate, DWORD dwError)
{
    Q_UNUSED(unMsg);
    qDebug()<<"Receive the new event "<<static_cast<int>(rasconnstate);
    switch (rasconnstate)
    {
    case RASCS_OpenPort:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_PortOpened:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_ConnectDevice:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_DeviceConnected:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_AllDevicesConnected:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_Authenticate:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_AuthNotify:
        //qDebug()<<__FUNCTION__ << __LINE__;
        if (dwError != 0) {
            //qDebug() << "have error" << dwError;
            setConnectionState(Disconnected);
        } else {
            //qDebug() << "RASCS_AuthNotify but no error" << dwError;
        }
        break;
    case RASCS_AuthRetry:
        //qDebug()<<__FUNCTION__ << __LINE__;
        setConnectionState(Preparing);
        break;
    case RASCS_AuthCallback:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_AuthChangePassword:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_AuthProject:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_AuthLinkSpeed:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_AuthAck:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_ReAuthenticate:
        //qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_Authenticated:
        //qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_PrepareForCallback:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_WaitForModemReset:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_WaitForCallback:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_Projected:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
#if (WINVER >= 0x400)
    case RASCS_StartAuthentication:    // Windows 95 only
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_CallbackComplete:       // Windows 95 only
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_LogonNetwork:           // Windows 95 only
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
#endif
    case RASCS_SubEntryConnected:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_SubEntryDisconnected:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
        //PAUSED STATES:
    case RASCS_Interactive:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_RetryAuthentication:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_CallbackSetByCaller:
        qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_PasswordExpired:
        setConnectionState(Error);
        qDebug()<<__FUNCTION__ << __LINE__;
        break;

    case RASCS_Connected: // = RASCS_DONE:
        setConnectionState(Connected);
        //qDebug()<<__FUNCTION__ << __LINE__;
        break;
    case RASCS_Disconnected:
        setConnectionState(Disconnected);
        //qDebug()<<__FUNCTION__ << __LINE__;
        break;
    default:
        //qDebug()<<__FUNCTION__ << __LINE__;
        break;
    }
}

void Ikev2Protocol::readIkev2Configuration(const QJsonObject &configuration)
{
    m_config = configuration.value(ProtocolProps::key_proto_config_data(Proto::Ikev2)).toObject();
}

ErrorCode Ikev2Protocol::start()
{
    QByteArray cert = QByteArray::fromBase64(m_config[config_key::cert].toString().toUtf8());
    setConnectionState(Connecting);

    QTemporaryFile certFile;
    certFile.setAutoRemove(false);
    certFile.open();
    certFile.write(cert);
    certFile.close();

    {
        auto certInstallProcess = IpcClient::CreatePrivilegedProcess();

        if (!certInstallProcess) {
            setLastError(ErrorCode::AmneziaServiceConnectionFailed);
            return ErrorCode::AmneziaServiceConnectionFailed;
        }

        certInstallProcess->waitForSource(1000);
        if (!certInstallProcess->isInitialized()) {
            qWarning() << "IpcProcess replica is not connected!";
            setLastError(ErrorCode::AmneziaServiceConnectionFailed);
            return ErrorCode::AmneziaServiceConnectionFailed;
        }
        certInstallProcess->setProgram(PermittedProcess::CertUtil);
        QStringList arguments({"-f" , "-importpfx",
                               "-p", m_config[config_key::password].toString(),
                               certFile.fileName(), "NoExport"
                              });
        certInstallProcess->setArguments(arguments);

        certInstallProcess->start();
    }
    // /*
    {
        if ( disconnect_vpn()){
            qDebug()<<"VPN was disconnected";
        }
        if ( delete_vpn_connection (tunnelName())){
            qDebug()<<"VPN was deleted";
        }
    }

    {
        {
            if ( !create_new_vpn(tunnelName(), m_config[config_key::hostName].toString())){
                qDebug() <<"Can't create the VPN connect";
            }
        }
    }

    {
        auto adapterConfigProcess = new QProcess;

        adapterConfigProcess->setProgram("powershell");
        QString arguments = QString("-command \"Set-VpnConnectionIPsecConfiguration\" "
                                    "-ConnectionName '%1' "
                                    "-AuthenticationTransformConstants GCMAES128 "
                                    "-CipherTransformConstants GCMAES128 "
                                    "-EncryptionMethod AES256 "
                                    "-IntegrityCheckMethod SHA256 "
                                    "-PfsGroup None "
                                    "-DHGroup Group14 "
                                    "-PassThru -Force\"")
                .arg(tunnelName());
        adapterConfigProcess->setNativeArguments(arguments);

        adapterConfigProcess->start();
        adapterConfigProcess->waitForFinished(5000);
    }
    //*/
    {
        if (!connect_to_vpn(tunnelName())) {
            qDebug()<<"We can't connect to VPN";
        }
    }
    //setConnectionState(Connecting);
    return ErrorCode::NoError;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::create_new_vpn(const QString & vpn_name,
                                   const QString & serv_addr){

    if ( RasValidateEntryName(nullptr, vpn_name.toStdWString().c_str()) != ERROR_SUCCESS)
        return false;
    DWORD size = 0;
    ::RasGetEntryProperties(nullptr, L"", nullptr, &size, nullptr, nullptr);
    LPRASENTRY pras = static_cast<LPRASENTRY>(malloc(size));
    memset(pras, 0, size);
    pras->dwSize = size;
    pras->dwType = RASET_Vpn;
    pras->dwRedialCount = 1;
    pras->dwRedialPause = 60;
    pras->dwfNetProtocols =  RASNP_Ip|RASNP_Ipv6;
    pras->dwEncryptionType = ET_RequireMax;
    wcscpy_s(pras->szLocalPhoneNumber, serv_addr.toStdWString().c_str());
    wcscpy_s(pras->szDeviceType, RASDT_Vpn);
    pras->dwfOptions = RASEO_RemoteDefaultGateway;
    pras->dwfOptions |= RASEO_RequireDataEncryption;
    pras->dwfOptions2 |= RASEO2_RequireMachineCertificates;
    pras->dwVpnStrategy = VS_Ikev2Only;
    const auto nRet = ::RasSetEntryProperties(nullptr, vpn_name.toStdWString().c_str(), pras, pras->dwSize, NULL, 0);
    free(pras);
    if (nRet == ERROR_SUCCESS)
        return true;
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::delete_vpn_connection(const QString &vpn_name){

    if ( RasDeleteEntry(nullptr, vpn_name.toStdWString().c_str()) == ERROR_SUCCESS){
        return true;
    }
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::connect_to_vpn(const QString & vpn_name){
    RASDIALPARAMS RasDialParams;
    memset(&RasDialParams, 0x0, sizeof(RASDIALPARAMS));
    RasDialParams.dwSize = sizeof(RASDIALPARAMS);
    wcscpy_s(RasDialParams.szEntryName, vpn_name.toStdWString().c_str());
    auto ret = RasDial(NULL, NULL, &RasDialParams, 0,
                       &RasDialFuncCallback,
                       &hRasConn);
    if (ret == ERROR_SUCCESS){
        return true;
    }
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::disconnect_vpn(){
    if ( hRasConn != nullptr ){
        if ( RasHangUp(hRasConn) != ERROR_SUCCESS)
            return false;
    }
    QThread::msleep(3000);

    return true;
}

void WINAPI RasDialFuncCallback(UINT unMsg,
                                RASCONNSTATE rasconnstate,
                                DWORD dwError ){
    std::lock_guard<std::mutex> guard(rasDialFuncMutex);
    if (self) {
        self->newConnectionStateEventReceived(unMsg, rasconnstate, dwError);
    }
}
