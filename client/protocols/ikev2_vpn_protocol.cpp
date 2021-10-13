#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
//#include <QRegularExpression>
//#include <QTcpSocket>
#include <QThread>

#include <chrono>

#include "debug.h"
#include "ikev2_vpn_protocol.h"
#include "utils.h"

static std::atomic<RASCONNSTATE>_connection_state;
static std::mutex mtx;
static std::condition_variable cv;
static std::atomic_bool newEvent{false};

Ikev2Protocol::Ikev2Protocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    //m_configFile.setFileTemplate(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    readIkev2Configuration(configuration);

    connect(_conn_state, &QTimer::timeout, this, &Ikev2Protocol::conn_state);
}

Ikev2Protocol::~Ikev2Protocol()
{
    qDebug() << "IpsecProtocol::~IpsecProtocol()";
#ifdef Q_OS_WIN
    _stoped.store(true);
    disconnect_vpn();
#endif
    Ikev2Protocol::stop();
    QThread::msleep(200);
    _thr->join();
}

void Ikev2Protocol::stop()
{
#ifdef Q_OS_WINDOWS
    {
        _stoped.store(true);
        //setConnectionState(Disconnecting);

        //auto disconnectProcess = new QProcess;

        //        disconnectProcess->setProgram("rasdial");
        //        QString arguments = QString("\"%1\" /disconnect")
        //                .arg(tunnelName());
        //        disconnectProcess->setNativeArguments(arguments);

        //        //        connect(connectProcess, &QProcess::readyRead, [connectProcess]() {
        //        //            qDebug().noquote() << "connectProcess readyRead" << connectProcess->readAll();
        //        //        });

        //        disconnectProcess->start();
        //        disconnectProcess->waitForFinished(5000);
        //        setConnectionState(Disconnected);
        if (! disconnect_vpn() ){
            qDebug()<<"We don't disconnect";
        }
    }
#endif
}

void Ikev2Protocol::readIkev2Configuration(const QJsonObject &configuration)
{
    m_config = configuration.value(ProtocolProps::key_proto_config_data(Protocol::Ikev2)).toObject();
}

ErrorCode Ikev2Protocol::start()
{
#ifdef Q_OS_WINDOWS
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
        certInstallProcess->setProgram("certutil");
        QStringList arguments({"-f" , "-importpfx",
                               "-p", m_config[config_key::password].toString(),
                               certFile.fileName(), "NoExport"
                              });
        certInstallProcess->setArguments(arguments);

        //        qDebug() << arguments.join(" ");
        //        connect(certInstallProcess.data(), &IpcProcessInterfaceReplica::errorOccurred, [certInstallProcess](QProcess::ProcessError error) {
        //            qDebug() << "IpcProcessInterfaceReplica errorOccurred" << error;
        //        });

        //        connect(certInstallProcess.data(), &IpcProcessInterfaceReplica::stateChanged, [certInstallProcess](QProcess::ProcessState newState) {
        //            qDebug() << "IpcProcessInterfaceReplica stateChanged" << newState;
        //        });

        //        connect(certInstallProcess.data(), &IpcProcessInterfaceReplica::readyRead, [certInstallProcess]() {
        //            auto req = certInstallProcess->readAll();
        //            req.waitForFinished();
        //            qDebug() << "IpcProcessInterfaceReplica readyRead" << req.returnValue();
        //        });


        certInstallProcess->start();
    }
    // /*
    {
        //        auto adapterRemoveProcess = new QProcess;

        //        adapterRemoveProcess->setProgram("powershell");
        //        QString arguments = QString("-command \"Remove-VpnConnection -Name '%1' -Force\"").arg(tunnelName());
        //        adapterRemoveProcess->setNativeArguments(arguments);

        //        adapterRemoveProcess->start();
        //        adapterRemoveProcess->waitForFinished(5000);
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
//        auto adapterInstallProcess = new QProcess;

//        adapterInstallProcess->setProgram("powershell");
//        QString arguments = QString("-command \"Add-VpnConnection "
//                                    "-ServerAddress '%1' "
//                                    "-Name '%2' "
//                                    "-TunnelType IKEv2 "
//                                    "-AuthenticationMethod MachineCertificate "
//                                    "-EncryptionLevel Required "
//                                    "-PassThru\"")
//                .arg(m_config[config_key::hostName].toString())
//                .arg(tunnelName());
//        adapterInstallProcess->setNativeArguments(arguments);
//        adapterInstallProcess->start();
//        adapterInstallProcess->waitForFinished(5000);
    }

    {
        auto adapterConfigProcess = new QProcess;

        adapterConfigProcess->setProgram("powershell");
        QString arguments = QString("-command \"Set-VpnConnectionIPsecConfiguration\ "
                                    "-ConnectionName '%1'\ "
                                    "-AuthenticationTransformConstants GCMAES128 "
                                    "-CipherTransformConstants GCMAES128 "
                                    "-EncryptionMethod AES256 "
                                    "-IntegrityCheckMethod SHA256 "
                                    "-PfsGroup None "
                                    "-DHGroup Group14 "
                                    "-PassThru -Force\"")
                .arg(tunnelName());
        adapterConfigProcess->setNativeArguments(arguments);

        //        connect(adapterConfigProcess, &QProcess::readyRead, [adapterConfigProcess]() {
        //            qDebug().noquote() << "adapterConfigProcess readyRead" << adapterConfigProcess->readAll();
        //        });

        adapterConfigProcess->start();
        adapterConfigProcess->waitForFinished(5000);
    }
    //*/
    {
        if (connect_to_vpn(tunnelName())){
            _thr = std::make_unique<std::thread>(&Ikev2Protocol::_ikev2_states, this);
        }else{
            qDebug()<<"We can't connect to VPN";
        }
    }
    //setConnectionState(Connecting);
    return ErrorCode::NoError;
#else
    return ErrorCode::NoError;
#endif

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Ikev2Protocol::conn_state(){

    if (hRasConn != nullptr){
        RASCONNSTATUS cs;
        cs.dwSize = sizeof(RASCONNSTATUS);
        RasGetConnectStatus(hRasConn, &cs);
        qDebug()<<"Current state RAS= "<< cs.rasconnstate;
        if (cs.rasconnstate == RASCS_DONE)//connected
        {
            setConnectionState(Connected);
        }
        if (cs.rasconnstate == 0)//disconnected
        {
            setConnectionState(Disconnected);
            _conn_state->stop();
        }
    }else{
        setConnectionState(Disconnected);
        _conn_state->stop();
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef Q_OS_WINDOWS
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
                       &Ikev2Protocol::RasDialFuncCallback,
                       &hRasConn);
    if (ret == ERROR_SUCCESS){
        //_th_conn_state = std::make_unique<std::thread>(&Ikev2Protocol::conn_state, this);
        _conn_state->start(5000);
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
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Ikev2Protocol::_ikev2_states(){

    while(!_stoped){
        qDebug()<<"We are waiting the change of RAS state";
        std::unique_lock<std::mutex> lck(mtx);
        while ( !newEvent ){
            cv.wait(lck);
        }
        qDebug()<<"Recive the new event "<<static_cast<int>(_connection_state);
        switch (_connection_state)
        {
        case RASCS_OpenPort:
            //setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_OpenPort = %d\n", _connection_state);
            //printf ("Opening port...\n");
            break;
        case RASCS_PortOpened:
            //setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_PortOpened = %d\n", _connection_state);
            //printf ("Port opened.\n");
            break;
        case RASCS_ConnectDevice:
            //setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_ConnectDevice = %d\n", _connection_state);
            //printf ("Connecting device...\n");
            break;
        case RASCS_DeviceConnected:
            //setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_DeviceConnected = %d\n", _connection_state);
            //printf ("Device connected.\n");
            break;
        case RASCS_AllDevicesConnected:
            //setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_AllDevicesConnected = %d\n", _connection_state);
            //printf ("All devices connected.\n");
            break;
        case RASCS_Authenticate:
            // setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_Authenticate = %d\n", _connection_state);
            // printf ("Authenticating...\n");
            break;
        case RASCS_AuthNotify:
            //setConnectionState(Disconnected);
            qDebug()<<__LINE__;
            //printf ("RASCS_AuthNotify = %d\n", _connection_state);
            // printf ("Authentication notify.\n");
            break;
        case RASCS_AuthRetry:
            // setConnectionState(Preparing);
            qDebug()<<__LINE__;
            //printf ("RASCS_AuthRetry = %d\n", _connection_state);
            //printf ("Retrying authentication...\n");
            break;
        case RASCS_AuthCallback:
            qDebug()<<__LINE__;
            //printf ("RASCS_AuthCallback = %d\n", _connection_state);
            //printf ("Authentication callback...\n");
            break;
        case RASCS_AuthChangePassword:
            qDebug()<<__LINE__;
            // printf ("RASCS_AuthChangePassword = %d\n", _connection_state);
            //printf ("Change password...\n");
            break;
        case RASCS_AuthProject:
            qDebug()<<__LINE__;
            //printf ("RASCS_AuthProject = %d\n", _connection_state);
            //printf ("Projection phase started...\n");
            break;
        case RASCS_AuthLinkSpeed:
            qDebug()<<__LINE__;
            //printf ("RASCS_AuthLinkSpeed = %d\n", _connection_state);
            //printf ("Negoting speed...\n");
            break;
        case RASCS_AuthAck:
            qDebug()<<__LINE__;
            //printf ("RASCS_AuthAck = %d\n", _connection_state);
            //printf ("Authentication acknowledge...\n");
            break;
        case RASCS_ReAuthenticate:
            qDebug()<<__LINE__;
            //printf ("RASCS_ReAuthenticate = %d\n", _connection_state);
            //printf ("Retrying Authentication...\n");
            break;
        case RASCS_Authenticated:
            qDebug()<<__LINE__;
            //printf ("RASCS_Authenticated = %d\n", _connection_state);
            //printf ("Authentication complete.\n");
            break;
        case RASCS_PrepareForCallback:
            qDebug()<<__LINE__;
            //printf ("RASCS_PrepareForCallback = %d\n", _connection_state);
            //printf ("Preparing for callback...\n");
            break;
        case RASCS_WaitForModemReset:
            qDebug()<<__LINE__;
            //printf ("RASCS_WaitForModemReset = %d\n", _connection_state);
            // printf ("Waiting for modem reset...\n");
            break;
        case RASCS_WaitForCallback:
            qDebug()<<__LINE__;
            //printf ("RASCS_WaitForCallback = %d\n", _connection_state);
            //printf ("Waiting for callback...\n");
            break;
        case RASCS_Projected:
            qDebug()<<__LINE__;
            //printf ("RASCS_Projected = %d\n", _connection_state);
            //printf ("Projection completed.\n");
            break;
#if (WINVER >= 0x400)
        case RASCS_StartAuthentication:    // Windows 95 only
            qDebug()<<__LINE__;
            //printf ("RASCS_StartAuthentication = %d\n", _connection_state);
            //printf ("Starting authentication...\n");

            break;
        case RASCS_CallbackComplete:       // Windows 95 only
            qDebug()<<__LINE__;
            //printf ("RASCS_CallbackComplete = %d\n", rasconnstate);
            //printf ("Callback complete.\n");
            break;
        case RASCS_LogonNetwork:           // Windows 95 only
            qDebug()<<__LINE__;
            //printf ("RASCS_LogonNetwork = %d\n", _connection_state);
            //printf ("Login to the network.\n");
            break;
#endif
        case RASCS_SubEntryConnected:
            qDebug()<<__LINE__;
            //printf ("RASCS_SubEntryConnected = %d\n", _connection_state);
            //printf ("Subentry connected.\n");
            break;
        case RASCS_SubEntryDisconnected:
            qDebug()<<__LINE__;
            //printf ("RASCS_SubEntryDisconnected = %d\n", _connection_state);
            //printf ("Subentry disconnected.\n");
            break;
            //PAUSED STATES:
        case RASCS_Interactive:
            qDebug()<<__LINE__;
            //printf ("RASCS_Interactive = %d\n", _connection_state);
            //printf ("In Paused state: Interactive mode.\n");
            break;
        case RASCS_RetryAuthentication:
            qDebug()<<__LINE__;
            //printf ("RASCS_RetryAuthentication = %d\n", _connection_state);
            //printf ("In Paused state: Retry Authentication...\n");
            break;
        case RASCS_CallbackSetByCaller:
            qDebug()<<__LINE__;
            //printf ("RASCS_CallbackSetByCaller = %d\n", _connection_state);
            //printf ("In Paused state: Callback set by Caller.\n");
            break;
        case RASCS_PasswordExpired:
            setConnectionState(Error);
            qDebug()<<__LINE__;
            //printf ("RASCS_PasswordExpired = %d\n", _connection_state);
            //printf ("In Paused state: Password has expired...\n");
            break;

        case RASCS_Connected: // = RASCS_DONE:
            //setConnectionState(Connected);
            qDebug()<<__LINE__;
            //printf ("RASCS_Connected = %d\n", _connection_state);
            //printf ("Connection completed.\n");
            //SetEvent(gEvent_handle);
            newEvent = false;
            return;
            break;
        case RASCS_Disconnected:
            //setConnectionState(Disconnected);
            qDebug()<<__LINE__;
            //printf ("RASCS_Disconnected = %d\n", _connection_state);
            //printf ("Disconnecting...\n");
            break;
        default:
            qDebug()<<__LINE__;
            //printf ("Unknown Status = %d\n", _connection_state);
            //printf ("What are you going to do about it?\n");
            break;
        }
        //reset RAS event
        newEvent = false;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#endif

#ifdef Q_OS_WINDOWS
void WINAPI Ikev2Protocol::RasDialFuncCallback(UINT /*unMsg*/,
                                               RASCONNSTATE rasconnstate,
                                               DWORD /*dwError*/ ){

    _connection_state = rasconnstate;
    newEvent = true;
    cv.notify_all();
}

#endif
