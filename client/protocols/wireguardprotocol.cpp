#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QThread>

#include "debug.h"
#include "wireguardprotocol.h"
#include "utils.h"

WireguardProtocol::WireguardProtocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    //m_configFile.setFileTemplate(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    m_configFile.setFileName(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    readWireguardConfiguration(configuration);
}

WireguardProtocol::~WireguardProtocol()
{
    //qDebug() << "WireguardProtocol::~WireguardProtocol() 1";
    WireguardProtocol::stop();
    QThread::msleep(200);
    //qDebug() << "WireguardProtocol::~WireguardProtocol() 2";
}

void WireguardProtocol::stop()
{
    //qDebug() << "WireguardProtocol::stop() 1";

#ifndef Q_OS_IOS
    if (!QFileInfo::exists(wireguardExecPath())) {
        qCritical() << "Wireguard executable missing!";
        setLastError(ErrorCode::ExecutableMissing);
        return;
    }

    m_wireguardStopProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_wireguardStopProcess) {
        qCritical() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    m_wireguardStopProcess->waitForSource(1000);
    if (!m_wireguardStopProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    m_wireguardStopProcess->setProgram(wireguardExecPath());


    QStringList arguments({"--remove", configPath()});
    m_wireguardStopProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");

    connect(m_wireguardStopProcess.data(), &PrivilegedProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol Stop errorOccurred" << error;
        setConnectionState(VpnConnectionState::Disconnected);
    });

    connect(m_wireguardStopProcess.data(), &PrivilegedProcess::stateChanged, this, [this](QProcess::ProcessState newState) {
        qDebug() << "WireguardProtocol::WireguardProtocol Stop stateChanged" << newState;
    });

    m_wireguardStopProcess->start();
    m_wireguardStopProcess->waitForFinished(10000);

    setConnectionState(VpnProtocol::Disconnected);
#endif

    //qDebug() << "WireguardProtocol::stop() 2";
}

void WireguardProtocol::readWireguardConfiguration(const QJsonObject &configuration)
{
    QJsonObject jConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toObject();

    if (!m_configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical() << "Failed to save wireguard config to" << m_configFile.fileName();
        return;
    }

    m_isConfigLoaded = true;

    m_configFile.write(jConfig.value(config_key::config).toString().toUtf8());
    m_configFile.close();
    m_configFileName = m_configFile.fileName();

    qDebug().noquote() << QString("Set config data") << m_configFileName;
    qDebug().noquote() << QString("Set config data") << configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toString().toUtf8();

}

QString WireguardProtocol::configPath() const
{
    return m_configFileName;
}

QString WireguardProtocol::wireguardExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable("wireguard/wireguard-service", true);
#elif defined Q_OS_LINUX
    return Utils::usrExecutable("wg");
#else
    return Utils::executable("/wireguard", true);
#endif
}

ErrorCode WireguardProtocol::start()
{
    //qDebug() << "WireguardProtocol::start() 1";

#ifndef Q_OS_IOS
    if (!m_isConfigLoaded) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    //qDebug() << "Start Wireguard connection";
    WireguardProtocol::stop();

    if (!QFileInfo::exists(wireguardExecPath())) {
        setLastError(ErrorCode::ExecutableMissing);
        return lastError();
    }

    if (!QFileInfo::exists(configPath())) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    setConnectionState(VpnConnectionState::Connecting);

    m_wireguardStartProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_wireguardStartProcess) {
        //qWarning() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_wireguardStartProcess->waitForSource(1000);
    if (!m_wireguardStartProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_wireguardStartProcess->setProgram(wireguardExecPath());


    QStringList arguments({"--add", configPath()});
    m_wireguardStartProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol errorOccurred" << error;
        setConnectionState(VpnConnectionState::Disconnected);
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::stateChanged, this, [this](QProcess::ProcessState newState) {
        qDebug() << "WireguardProtocol::WireguardProtocol stateChanged" << newState;
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::finished, this, [&]() {
        setConnectionState(VpnConnectionState::Connected);
        {
            //TODO:FIXME: without some ugly sleep we have't get a adapter parametrs
            std::this_thread::sleep_for(std::chrono::seconds(4));
            std::string p1{},p2{},p3;
            const auto &ret = adpInfo.get_adapter_info("WireGuard Tunnel");//serviceName().toStdString());//("AmneziaVPN IKEv2");
            if (std::get<0>(ret) == false){
                p1 = adpInfo.get_adapter_route_gateway();
                p2 = adpInfo.get_adapter_local_address();
                p3 = adpInfo.get_adapter_local_gateway();
                m_routeGateway = QString::fromStdString(p1);
                m_vpnLocalAddress = QString::fromStdString(p2);
                m_vpnGateway = QString::fromStdString(p3);
                qDebug()<<"My wireguard m_routeGateway "<<m_routeGateway;
                qDebug()<<"My wireguard m_vpnLocalAddress "<<m_vpnLocalAddress;
                qDebug()<<"My wireguard m_vpnGateway "<< m_vpnGateway;
                auto ret = adpinfo::get_route_table(p2.c_str());
                {
                    for (const auto &itret: ret){
                        const auto ip = itret.szDestIp;//std::get<0>(itret);
                        const auto msk = itret.szMaskIp;//std::get<1>(itret);
                        const auto gw = itret.szGatewayIp;//std::get<2>(itret);
                        const auto itf = itret.szInterfaceIp;//std::get<3>(itret);
                        const auto itfInd = itret.ulIfIndex;
                        qDebug()<<"IP["<<ip.c_str()<<"]"<<"Mask["<<msk.c_str()<<"]"<<"gateway["<<gw.c_str()<<"]"<<"Interface["<<itf.c_str()<<"]"<<"Interface index["<<itfInd<<"]";
                        emit route_avaible(QString::fromStdString(ip),
                                           QString::fromStdString(msk),
                                           QString::fromStdString(gw),
                                           QString::fromStdString(itf),
                                           itfInd);
                        //m_vpnGateway = QString::fromStdString(gw);
                    }
                }
                //qDebug()<<"My wireguard m_routeGateway "<<m_routeGateway;
                //qDebug()<<"My wireguard m_vpnLocalAddress "<<m_vpnLocalAddress;
                //qDebug()<<"My wireguard m_vpnGateway "<< m_vpnGateway;
            }
            else{
                qDebug()<<"We can't get information about active adapter:"<<QString::fromStdString(std::get<1>(ret));
            }
        }
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::readyRead, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAll();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readyRead" << reply.returnValue();
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::readyReadStandardOutput, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAllStandardOutput();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardOutput" << reply.returnValue();
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::readyReadStandardError, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAllStandardError();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardError" << reply.returnValue();
    });

    m_wireguardStartProcess->start();
    m_wireguardStartProcess->waitForFinished(10000);

    //qDebug() << "WireguardProtocol::start() 2";

    return ErrorCode::NoError;
#else
    return ErrorCode::NotImplementedError;
#endif
}

QString WireguardProtocol::serviceName() const
{
    return "AmneziaVPN.WireGuard0";
}
