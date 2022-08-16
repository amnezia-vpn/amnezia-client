#include "openvpnovercloakprotocol.h"
#include "core/servercontroller.h"

#include "utils.h"
#include "containers/containers_defs.h"

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
#ifndef Q_OS_IOS
    m_ckProcess.close();
#endif
}

ErrorCode OpenVpnOverCloakProtocol::start()
{
    if (!QFileInfo::exists(cloakExecPath())) {
        setLastError(ErrorCode::CloakExecutableMissing);
        return lastError();
    }
#ifndef Q_OS_IOS
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

    ProtocolEnumNS::TransportProto tp = ProtocolProps::transportProtoFromString(m_cloakConfig.value(config_key::transport_proto).toString());
    if (tp == ProtocolEnumNS::TransportProto::Udp) {
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
        setConnectionState(VpnConnectionState::Connecting);

        return OpenVpnProtocol::start();
    }
    else return ErrorCode::CloakExecutableMissing;
#endif
}

void OpenVpnOverCloakProtocol::stop()
{
    disconnect(m_errorHandlerConnection);
    OpenVpnProtocol::stop();

    qDebug() << "OpenVpnOverCloakProtocol::stop()";

#ifdef Q_OS_WIN
    Utils::signalCtrl(m_ckProcess.processId(), CTRL_C_EVENT);
#endif

#ifndef Q_OS_IOS
    m_ckProcess.terminate();
#endif
}

QString OpenVpnOverCloakProtocol::cloakExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("cloak/ck-client"), true);
#elif defined Q_OS_LINUX
        return Utils::usrExecutable("ck-client");
#else
    return Utils::executable(QString("/ck-client"), true);
#endif
}

void OpenVpnOverCloakProtocol::readCloakConfiguration(const QJsonObject &configuration)
{
    m_cloakConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::Cloak)).toObject();
}
