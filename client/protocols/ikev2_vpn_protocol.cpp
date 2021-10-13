#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QThread>

#include "debug.h"
#include "ikev2_vpn_protocol.h"
#include "utils.h"


Ikev2Protocol::Ikev2Protocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    //m_configFile.setFileTemplate(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    readIkev2Configuration(configuration);
}

Ikev2Protocol::~Ikev2Protocol()
{
    qDebug() << "IpsecProtocol::~IpsecProtocol()";
    Ikev2Protocol::stop();
    QThread::msleep(200);
}

void Ikev2Protocol::stop()
{
#ifdef Q_OS_WINDOWS
    {
        setConnectionState(Disconnecting);

        auto disconnectProcess = new QProcess;

        disconnectProcess->setProgram("rasdial");
        QString arguments = QString("\"%1\" /disconnect")
                                .arg(tunnelName());
        disconnectProcess->setNativeArguments(arguments);

//        connect(connectProcess, &QProcess::readyRead, [connectProcess]() {
//            qDebug().noquote() << "connectProcess readyRead" << connectProcess->readAll();
//        });

        disconnectProcess->start();
        disconnectProcess->waitForFinished(5000);
        setConnectionState(Disconnected);
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
    setConnectionState(ConnectionState::Connecting);

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

    {
        auto adapterRemoveProcess = new QProcess;

        adapterRemoveProcess->setProgram("powershell");
        QString arguments = QString("-command \"Remove-VpnConnection -Name '%1' -Force\"").arg(tunnelName());
        adapterRemoveProcess->setNativeArguments(arguments);

        adapterRemoveProcess->start();
        adapterRemoveProcess->waitForFinished(5000);
    }

    {
        auto adapterInstallProcess = new QProcess;

        adapterInstallProcess->setProgram("powershell");
        QString arguments = QString("-command \"Add-VpnConnection "
                                    "-ServerAddress '%1' "
                                    "-Name '%2' "
                                    "-TunnelType IKEv2 "
                                    "-AuthenticationMethod MachineCertificate "
                                    "-EncryptionLevel Required "
                                    "-PassThru\"")
                                .arg(m_config[config_key::hostName].toString())
                                .arg(tunnelName());
        adapterInstallProcess->setNativeArguments(arguments);
//        connect(adapterInstallProcess, &QProcess::readyRead, [adapterInstallProcess]() {
//            qDebug().noquote() << "adapterInstallProcess readyRead" << adapterInstallProcess->readAll();
//        });

        adapterInstallProcess->start();
        adapterInstallProcess->waitForFinished(5000);
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

    {
//        char buf[RASBUFFER]= {0};
//        DWORD err = 0;
//        RASDIALPARAMSA *param = (RASDIALPARAMSA *)buf;
//        param->dwSize = 1064;
//        strcpy(param->szEntryName, tunnelName().toStdString().c_str());
//        err = RasDialA(NULL, NULL, param, 0, (LPVOID)rasCallback, &g_h);
//        qDebug() << "Ikev2Protocol::start() ret" << err;


        auto connectProcess = new QProcess;

        connectProcess->setProgram("rasdial");
        QString arguments = QString("\"%1\"")
                                .arg(tunnelName());
        connectProcess->setNativeArguments(arguments);

        connect(connectProcess, &QProcess::readyRead, [connectProcess]() {
            qDebug().noquote() << "connectProcess readyRead" << connectProcess->readAll();
        });

        connectProcess->start();
        connectProcess->waitForFinished(5000);
    }

    setConnectionState(Connected);
    return ErrorCode::NoError;

#endif

    return ErrorCode::NoError;
}

#ifdef Q_OS_WINDOWS
DWORD CALLBACK rasCallback(UINT msg, RASCONNSTATE rascs, DWORD err)
{
    if(err != 0) {
        printf("Error: %d\n", err);
        fflush(stdout);
        //g_done = 1;
        return 0;   // stop the connection.
    } else {
        //printf("%s\n", rasState(rascs));
        fflush(stdout);
        if(rascs == RASCS_Connected) {
            printf("Success: Connected\n");
            fflush(stdout);
            //g_done = 1;
        }
        return 1;
    }
}
#endif
