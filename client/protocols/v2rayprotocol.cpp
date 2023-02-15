#include "v2rayprotocol.h"

#include <QThread>
#include <QFileInfo>
#include <QJsonDocument>

#include "utilities.h"

V2RayProtocol::V2RayProtocol(const QJsonObject &configuration, QObject *parent) : OpenVpnProtocol(configuration, parent)
{
    writeV2RayConfiguration(configuration);
}

V2RayProtocol::~V2RayProtocol()
{
    qDebug() << "V2RayProtocol::~V2RayProtocol";
    V2RayProtocol::stop();
    QThread::msleep(200);
}

ErrorCode V2RayProtocol::start()
{
#ifndef Q_OS_IOS
    if (!QFileInfo::exists(v2RayExecPath())) {
        setLastError(ErrorCode::V2RayExecutableMissing);
        return lastError();
    }

    if (Utils::processIsRunning(Utils::executable("v2ray", false))) {
        Utils::killProcessByName(Utils::executable("v2ray", false));
    }

    QStringList args = QStringList() << "-c" << m_v2RayConfigFile.fileName();

    qDebug().noquote() << "V2RayProtocol::start()" << v2RayExecPath() << args.join(" ");

    m_v2RayProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_v2RayProcess.setProgram(v2RayExecPath());
    m_v2RayProcess.setArguments(args);

    connect(&m_v2RayProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        qDebug().noquote() << "V2Ray:" << m_v2RayProcess.readAllStandardOutput();
    });

    connect(&m_v2RayProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug().noquote() << "V2RayProtocol finished, exitCode, exiStatus" << exitCode << exitStatus;
        setConnectionState(VpnProtocol::Disconnected);
        if (exitStatus != QProcess::NormalExit) {
            emit protocolError(amnezia::ErrorCode::V2RayExecutableCrashed);
            stop();
        }
        if (exitCode != 0 ) {
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_v2RayProcess.start();
    m_v2RayProcess.waitForStarted();

    if (m_v2RayProcess.state() == QProcess::ProcessState::Running) {
        setConnectionState(VpnConnectionState::Connecting);

        return OpenVpnProtocol::start();
    } else {
        return ErrorCode::V2RayExecutableMissing;
    }
#else
    return ErrorCode::NotImplementedError;
#endif
    return ErrorCode::NoError;
}

void V2RayProtocol::stop()
{
    OpenVpnProtocol::stop();

    qDebug() << "V2RayProtocol::stop()";
#ifndef Q_OS_IOS
    m_v2RayProcess.terminate();
#endif

#ifdef Q_OS_WIN
    Utils::signalCtrl(m_v2RayProcess.processId(), CTRL_C_EVENT);
#endif
}

void V2RayProtocol::writeV2RayConfiguration(const QJsonObject &configuration)
{
    m_v2RayConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::V2Ray)).toObject();

#ifdef QT_DEBUG
    m_v2RayConfigFile.setAutoRemove(false);
#endif
    m_v2RayConfigFile.open();
    m_v2RayConfigFile.write(QJsonDocument(m_v2RayConfig).toJson());
    m_v2RayConfigFile.close();
}

const QString V2RayProtocol::v2RayExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("v2ray/v2ray"), true);
#else
    return Utils::executable(QString("v2ray"), true);
#endif
}
