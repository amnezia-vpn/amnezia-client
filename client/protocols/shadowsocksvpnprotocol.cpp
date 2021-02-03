#include "shadowsocksvpnprotocol.h"
#include "core/servercontroller.h"

//#include "communicator.h"
#include "debug.h"
#include "utils.h"

#include <QJsonDocument>
#include <QJsonObject>

ShadowSocksVpnProtocol::ShadowSocksVpnProtocol(const QString &args, QObject *parent):
    OpenVpnProtocol(args, parent)
{
    m_shadowSocksConfig = args;
}

ErrorCode ShadowSocksVpnProtocol::start()
{
    qDebug() << "ShadowSocksVpnProtocol::start()";
    QJsonObject config = QJsonDocument::fromJson(m_shadowSocksConfig.toUtf8()).object();

    ssProcess.setProcessChannelMode(QProcess::MergedChannels);

    ssProcess.setProgram(shadowSocksExecPath());
    ssProcess.setArguments(QStringList() << "-s" << config.value("server").toString()
                                 << "-p" << QString::number(config.value("server_port").toInt())
                                 << "-l" << QString::number(config.value("local_port").toInt())
                                 << "-m" << config.value("method").toString()
                                 << "-k" << config.value("password").toString()
    );

    ssProcess.start();
    ssProcess.waitForStarted();

    if (ssProcess.state() == QProcess::ProcessState::Running) {
        setConnectionState(ConnectionState::Connecting);

        return OpenVpnProtocol::start();
    }
    else return ErrorCode::FailedToStartRemoteProcessError;
}

void ShadowSocksVpnProtocol::stop()
{
    qDebug() << "ShadowSocksVpnProtocol::stop()";
    ssProcess.kill();
}

QString ShadowSocksVpnProtocol::shadowSocksExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("ss/ss-local"), true);
#else
    return Utils::executable(QString("/ss-local"), true);
#endif
}

QString ShadowSocksVpnProtocol::genShadowSocksConfig(const ServerCredentials &credentials, Protocol proto)
{
    QJsonObject ssConfig;
    ssConfig.insert("server", credentials.hostName);
    ssConfig.insert("server_port", ServerController::ssRemotePort());
    ssConfig.insert("local_port", ServerController::ssContainerPort());
    ssConfig.insert("password", credentials.password);
    ssConfig.insert("timeout", 60);
    ssConfig.insert("method", ServerController::ssEncryption());
    return QJsonDocument(ssConfig).toJson();
}
