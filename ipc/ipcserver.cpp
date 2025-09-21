#include "ipcserver.h"

#include <QDateTime>
#include <QFileInfo>
#include <QLocalSocket>
#include <QObject>
#include <QJsonArray>

#include "logger.h"
#include "router.h"

#include "killswitch.h"

#ifdef Q_OS_WIN
    #include "tapcontroller_win.h"
#endif


IpcServer::IpcServer(QObject *parent) : IpcInterfaceSource(parent)

{
}

int IpcServer::createPrivilegedProcess()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::createPrivilegedProcess";
#endif

    m_localpid++;

    ProcessDescriptor pd(this);

    pd.localServer->setSocketOptions(QLocalServer::WorldAccessOption);

    if (!pd.localServer->listen(amnezia::getIpcProcessUrl(m_localpid))) {
        qDebug() << QString("Unable to start the server: %1.").arg(pd.localServer->errorString());
        return -1;
    }

    // Make sure any connections are handed to QtRO
    QObject::connect(pd.localServer.data(), &QLocalServer::newConnection, this, [pd]() {
        qDebug() << "IpcServer new connection";
        if (pd.serverNode) {
            pd.serverNode->addHostSideConnection(pd.localServer->nextPendingConnection());
            pd.serverNode->enableRemoting(pd.ipcProcess.data());
        }
    });

    QObject::connect(pd.serverNode.data(), &QRemoteObjectHost::error, this,
                     [pd](QRemoteObjectNode::ErrorCode errorCode) { qDebug() << "QRemoteObjectHost::error" << errorCode; });

    QObject::connect(pd.serverNode.data(), &QRemoteObjectHost::destroyed, this, [pd]() { qDebug() << "QRemoteObjectHost::destroyed"; });

    m_processes.insert(m_localpid, pd);

    return m_localpid;
}

int IpcServer::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::routeAddList";
#endif

    return Router::routeAddList(gw, ips);
}

bool IpcServer::clearSavedRoutes()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::clearSavedRoutes";
#endif

    return Router::clearSavedRoutes();
}

bool IpcServer::routeDeleteList(const QString &gw, const QStringList &ips)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::routeDeleteList";
#endif

    return Router::routeDeleteList(gw, ips);
}

void IpcServer::flushDns()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::flushDns";
#endif

    return Router::flushDns();
}

void IpcServer::resetIpStack()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::resetIpStack";
#endif

    Router::resetIpStack();
}

bool IpcServer::checkAndInstallDriver()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::checkAndInstallDriver";
#endif

#ifdef Q_OS_WIN
    return TapController::checkAndSetup();
#else
    return true;
#endif
}

QStringList IpcServer::getTapList()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::getTapList";
#endif

#ifdef Q_OS_WIN
    return TapController::getTapList();
#else
    return QStringList();
#endif
}

void IpcServer::cleanUp()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::cleanUp";
#endif

    Logger::deInit();
    Logger::cleanUp();
}

void IpcServer::clearLogs()
{
    Logger::clearLogs(true);
}

bool IpcServer::createTun(const QString &dev, const QString &subnet)
{
    return Router::createTun(dev, subnet);
}

bool IpcServer::deleteTun(const QString &dev)
{
    return Router::deleteTun(dev);
}

bool IpcServer::updateResolvers(const QString &ifname, const QList<QHostAddress> &resolvers)
{
    return Router::updateResolvers(ifname, resolvers);
}

void IpcServer::StartRoutingIpv6()
{
    Router::StartRoutingIpv6();
}

void IpcServer::StopRoutingIpv6()
{
    Router::StopRoutingIpv6();
}

void IpcServer::setLogsEnabled(bool enabled)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::setLogsEnabled";
#endif

    if (enabled) {
        Logger::init(true);
    } else {
        Logger::deInit();
    }
}

bool IpcServer::resetKillSwitchAllowedRange(QStringList ranges)
{
    return KillSwitch::instance()->resetAllowedRange(ranges);
}

bool IpcServer::addKillSwitchAllowedRange(QStringList ranges)
{
    return KillSwitch::instance()->addAllowedRange(ranges);
}

bool IpcServer::disableAllTraffic()
{
    return KillSwitch::instance()->disableAllTraffic();
}

bool IpcServer::enableKillSwitch(const QJsonObject &configStr, int vpnAdapterIndex)
{
    return KillSwitch::instance()->enableKillSwitch(configStr, vpnAdapterIndex);
}

bool IpcServer::disableKillSwitch()
{
    return KillSwitch::instance()->disableKillSwitch();
}

bool IpcServer::startIPsec(QString tunnelName)
{
#ifdef Q_OS_LINUX
    QProcess processSystemd;
    QStringList commandsSystemd;
    commandsSystemd << "systemctl" << "restart" << "ipsec";
    processSystemd.start("sudo", commandsSystemd);
    if (!processSystemd.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel!\n";
        return false;
    }
    else if (!processSystemd.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel\n";
        return false;
    }
    commandsSystemd.clear();

    QThread::msleep(5000);

    QProcess process;
    QStringList commands;
    commands << "ipsec" << "up" << QString("%1").arg(tunnelName);
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel!\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel\n";
        return false;
    }
    commands.clear();
#endif
    return true;
}

bool IpcServer::stopIPsec(QString tunnelName)
{
#ifdef Q_OS_LINUX
    QProcess process;
    QStringList commands;
    commands << "ipsec" << "down" << QString("%1").arg(tunnelName);
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return false;
    }
    commands.clear();
#endif
    return true;
}

bool IpcServer::writeIPsecConfig(QString config)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: IPSec config file";
    QString configFile = QString("/etc/ipsec.conf");
    QFile ipSecConfFile(configFile);
    if (ipSecConfFile.open(QIODevice::WriteOnly)) {
        ipSecConfFile.write(config.toUtf8());
        ipSecConfFile.close();
    }
#endif
    return true;
}

bool IpcServer::writeIPsecUserCert(QString usercert, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: Write user cert " << uuid;
    QString certName = QString("/etc/ipsec.d/certs/%1.crt").arg(uuid);
    QFile userCertFile(certName);
    if (userCertFile.open(QIODevice::WriteOnly)) {
        userCertFile.write(usercert.toUtf8());
        userCertFile.close();
    }
#endif
    return true;
}

bool IpcServer::writeIPsecCaCert(QString cacert, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: Write CA cert user " << uuid;
    QString certName = QString("/etc/ipsec.d/cacerts/%1.crt").arg(uuid);
    QFile caCertFile(certName);
    if (caCertFile.open(QIODevice::WriteOnly)) {
        caCertFile.write(cacert.toUtf8());
        caCertFile.close();
    }
#endif
    return true;
}

bool IpcServer::writeIPsecPrivate(QString privKey, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: User private key " << uuid;
    QString privateKey = QString("/etc/ipsec.d/private/%1.p12").arg(uuid);
    QFile pKeyFile(privateKey);
    if (pKeyFile.open(QIODevice::WriteOnly)) {
        pKeyFile.write(QByteArray::fromBase64(privKey.toUtf8()));
        pKeyFile.close();
    }
#endif
    return true;
}


bool IpcServer::writeIPsecPrivatePass(QString pass, QString host, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: User private key " << uuid;
    const QString secretsFilename = "/etc/ipsec.secrets";
    QStringList lines;

    {
        QFile secretsFile(secretsFilename);
        if (secretsFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream edit(&secretsFile);
            while (!edit.atEnd()) lines.push_back(edit.readLine());
        }
        secretsFile.close();
    }

    for (auto iter = lines.begin(); iter!=lines.end();)
    {
        if (iter->contains(host))
        {
            iter = lines.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    {
        QFile secretsFile(secretsFilename);
        if (secretsFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream edit(&secretsFile);
            for (int i=0; i<lines.size(); i++) edit << lines[i] << Qt::endl;
        }
        QString P12 = QString("%any %1 : P12 %2.p12 \"%3\" \n").arg(host, uuid, pass);
        secretsFile.write(P12.toUtf8());
        secretsFile.close();
    }
#endif
    return true;
}

QString IpcServer::getTunnelStatus(QString tunnelName)
{
#ifdef Q_OS_LINUX
    QProcess process;
    QStringList commands;
    commands << "ipsec" << "status" << QString("%1").arg(tunnelName);
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return "";
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return "";
    }
    commands.clear();


    QString status = process.readAll();
    return status;
#endif
    return QString();

}

bool IpcServer::enablePeerTraffic(const QJsonObject &configStr)
{
    return KillSwitch::instance()->enablePeerTraffic(configStr);
}

bool IpcServer::refreshKillSwitch(bool enabled)
{
    return KillSwitch::instance()->refresh(enabled);
}
