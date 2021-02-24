#include "shadowsocksvpnprotocol.h"
#include "core/servercontroller.h"

#include "debug.h"
#include "utils.h"

#include <QJsonDocument>
#include <QJsonObject>

ShadowSocksVpnProtocol::ShadowSocksVpnProtocol(const QJsonObject &configuration, QObject *parent):
    OpenVpnProtocol(configuration, parent)
{
    readShadowSocksConfiguration(configuration);
}

ShadowSocksVpnProtocol::~ShadowSocksVpnProtocol()
{
    qDebug() << "ShadowSocksVpnProtocol::stop()";
    ShadowSocksVpnProtocol::stop();
}

ErrorCode ShadowSocksVpnProtocol::start()
{
    qDebug() << "ShadowSocksVpnProtocol::start()";

    m_ssProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_ssProcess.setProgram(shadowSocksExecPath());
    m_ssProcess.setArguments(QStringList() << "-s" << m_shadowSocksConfig.value("server").toString()
                                 << "-p" << QString::number(m_shadowSocksConfig.value("server_port").toInt())
                                 << "-l" << QString::number(m_shadowSocksConfig.value("local_port").toInt())
                                 << "-m" << m_shadowSocksConfig.value("method").toString()
                                 << "-k" << m_shadowSocksConfig.value("password").toString()
    );

    connect(&m_ssProcess, &QProcess::readyRead, this, [this](){
        qDebug().noquote() << m_ssProcess.readAll();
    });

    m_ssProcess.start();
    m_ssProcess.waitForStarted();

    if (m_ssProcess.state() == QProcess::ProcessState::Running) {
        setConnectionState(ConnectionState::Connecting);

        return OpenVpnProtocol::start();
    }
    else return ErrorCode::ShadowSocksExecutableMissing;
}

void ShadowSocksVpnProtocol::stop()
{
    OpenVpnProtocol::stop();

    qDebug() << "ShadowSocksVpnProtocol::stop()";
    m_ssProcess.close();
}

QString ShadowSocksVpnProtocol::shadowSocksExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("ss/ss-local"), true);
#else
    return Utils::executable(QString("/ss-local"), true);
#endif
}

QJsonObject ShadowSocksVpnProtocol::genShadowSocksConfig(const ServerCredentials &credentials, Protocol proto)
{
    QJsonObject ssConfig;
    ssConfig.insert("server", credentials.hostName);
    ssConfig.insert("server_port", ServerController::ssRemotePort());
    ssConfig.insert("local_port", ServerController::ssContainerPort());
    ssConfig.insert("password", credentials.password);
    ssConfig.insert("timeout", 60);
    ssConfig.insert("method", ServerController::ssEncryption());
    return ssConfig;
}

void ShadowSocksVpnProtocol::readShadowSocksConfiguration(const QJsonObject &configuration)
{
    m_shadowSocksConfig = configuration.value(config::key_shadowsocks_config_data()).toObject();
}
