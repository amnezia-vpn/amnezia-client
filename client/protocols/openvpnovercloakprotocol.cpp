#include "openvpnovercloakprotocol.h"
#include "core/servercontroller.h"

#include "utils.h"
#include "protocols/protocols_defs.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

OpenVpnOverCloakProtocol::OpenVpnOverCloakProtocol(const QJsonObject &configuration, QObject *parent):
    OpenVpnProtocol(configuration, parent)
{
    readCloakConfiguration(configuration);
}

OpenVpnOverCloakProtocol::~OpenVpnOverCloakProtocol()
{
    qDebug() << "OpenVpnOverCloakProtocol::~OpenVpnOverCloakProtocol";
    OpenVpnOverCloakProtocol::stop();
    QThread::msleep(200);
    m_ckProcess.close();
}

ErrorCode OpenVpnOverCloakProtocol::start()
{
    if (Utils::processIsRunning(Utils::executable("ck-client", false))) {
        Utils::killProcessByName(Utils::executable("ck-client", false));
    }

#ifdef QT_DEBUG
    m_cloakCfgFile.setAutoRemove(false);
#endif
    m_cloakCfgFile.open();
    m_cloakCfgFile.write(QJsonDocument(m_cloakConfig).toJson());
    m_cloakCfgFile.close();

    QStringList args = QStringList() << "-c" << m_cloakCfgFile.fileName()
                                     << "-s" << m_cloakConfig.value(config_key::remote).toString()
                                     << "-p" << m_cloakConfig.value(config_key::port).toString(amnezia::protocols::cloak::defaultPort)
                                     << "-l" << amnezia::protocols::openvpn::defaultPort;

    if (m_cloakConfig.value(config_key::transport_proto).toString() == protocols::UDP) {
        args << "-u";
    }

    qDebug().noquote() << "OpenVpnOverCloakProtocol::start()"
                       << cloakExecPath() << args.join(" ");

    m_ckProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_ckProcess.setProgram(cloakExecPath());
    m_ckProcess.setArguments(args);

    connect(&m_ckProcess, &QProcess::readyReadStandardOutput, this, [this](){
        qDebug().noquote() << "ck-client:" << m_ckProcess.readAllStandardOutput();
    });

    m_errorHandlerConnection = connect(&m_ckProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus){
        qDebug().noquote() << "OpenVpnOverCloakProtocol finished, exitCode, exiStatus" << exitCode << exitStatus;
        setConnectionState(VpnProtocol::Disconnected);
        if (exitStatus != QProcess::NormalExit){
            emit protocolError(amnezia::ErrorCode::CloakExecutableCrashed);
            stop();
        }
        if (exitCode !=0 ){
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_ckProcess.start();
    m_ckProcess.waitForStarted();

    if (m_ckProcess.state() == QProcess::ProcessState::Running) {
        setConnectionState(ConnectionState::Connecting);

        return OpenVpnProtocol::start();
    }
    else return ErrorCode::CloakExecutableMissing;
}

void OpenVpnOverCloakProtocol::stop()
{
    disconnect(m_errorHandlerConnection);
    OpenVpnProtocol::stop();

    qDebug() << "OpenVpnOverCloakProtocol::stop()";

#ifdef Q_OS_WIN
    Utils::signalCtrl(m_ckProcess.processId(), CTRL_C_EVENT);
#endif

    m_ckProcess.terminate();
}

QString OpenVpnOverCloakProtocol::cloakExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("cloak/ck-client"), true);
#else
    return Utils::executable(QString("/ck-client"), true);
#endif
}

void OpenVpnOverCloakProtocol::readCloakConfiguration(const QJsonObject &configuration)
{
    m_cloakConfig = configuration.value(config::key_cloak_config_data).toObject();
}
