#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QJsonObject>

#include <configurators/openvpn_configurator.h>
#include <configurators/cloak_configurator.h>
#include <configurators/shadowsocks_configurator.h>
#include <core/servercontroller.h>

#include "ipc.h"
#include "core/ipcclient.h"
#include "protocols/openvpnprotocol.h"
#include "protocols/openvpnovercloakprotocol.h"
#include "protocols/shadowsocksvpnprotocol.h"

#include "utils.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(QObject* parent) : QObject(parent)
{
    QTimer::singleShot(0, this, [this](){
        if (!IpcClient::init()) {
            qWarning() << "Error occured when init IPC client";
            emit serviceIsNotReady();
        }
    });
}

VpnConnection::~VpnConnection()
{
    m_vpnProtocol.clear();
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    if (IpcClient::Interface()) {
        if (state == VpnProtocol::ConnectionState::Connected && IpcClient::Interface()){
            IpcClient::Interface()->flushDns();

            if (m_settings.customRouting()) {
                IpcClient::Interface()->routeDelete("0.0.0.0", m_vpnProtocol->vpnGateway());

                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(),
                    QStringList() << m_settings.primaryDns() << m_settings.secondaryDns());

                const QStringList &black_custom = m_settings.customIps();
                qDebug() << "VpnConnection::onConnectionStateChanged :: adding custom routes, count:" << black_custom.size();

                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), black_custom);
            }
        }
        else if (state == VpnProtocol::ConnectionState::Error) {
            IpcClient::Interface()->flushDns();

            if (m_settings.customRouting()) {
                IpcClient::Interface()->clearSavedRoutes();
            }
        }
    }

    emit connectionStateChanged(state);
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_vpnProtocol;
}

ErrorCode VpnConnection::lastError() const
{
    if (!m_vpnProtocol.data()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

QMap<Protocol, QString> VpnConnection::getLastVpnConfig(const QJsonObject &containerConfig)
{
    QMap<Protocol, QString> configs;
    for (Protocol proto: { Protocol::OpenVpn,
         Protocol::ShadowSocks,
         Protocol::Cloak,
         Protocol::WireGuard}) {

        QString cfg = containerConfig.value(protoToString(proto)).toObject().value(config_key::last_config).toString();

        if (!cfg.isEmpty()) configs.insert(proto, cfg);
    }
    return configs;
}

QString VpnConnection::createVpnConfigurationForProto(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig, Protocol proto,
    ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;
    auto lastVpnConfig = getLastVpnConfig(containerConfig);

    QString configData;
    if (lastVpnConfig.contains(proto)) {
        configData = lastVpnConfig.value(proto);
        qDebug() << "VpnConnection::createVpnConfiguration using saved config for " << protoToString(proto);
    }
    else {
        if (proto == Protocol::OpenVpn) {
            configData = OpenVpnConfigurator::genOpenVpnConfig(credentials,
                container, containerConfig, &e);
        }
        else if (proto == Protocol::Cloak) {
            configData = CloakConfigurator::genCloakConfig(credentials,
                container, containerConfig, &e);
        }
        else if (proto == Protocol::ShadowSocks) {
            configData = ShadowSocksConfigurator::genShadowSocksConfig(credentials,
                container, containerConfig, &e);
        }

        if (errorCode && e) {
            *errorCode = e;
            return "";
        }


        if (serverIndex >= 0) {
            QJsonObject protoObject = m_settings.protocolConfig(serverIndex, container, proto);
            protoObject.insert(config_key::last_config, configData);
            m_settings.setProtocolConfig(serverIndex, container, proto, protoObject);
        }
    }

    if (errorCode) *errorCode = e;
    return configData;
}

ErrorCode VpnConnection::createVpnConfiguration(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (container == DockerContainer::OpenVpn ||
            container == DockerContainer::OpenVpnOverShadowSocks ||
            container == DockerContainer::OpenVpnOverCloak) {

        QString openVpnConfigData =
            createVpnConfigurationForProto(
                serverIndex, credentials, container, containerConfig, Protocol::OpenVpn, &errorCode);


        m_vpnConfiguration.insert(config::key_openvpn_config_data, openVpnConfigData);
        if (errorCode) {
            return errorCode;
        }

        QFile file(Utils::defaultVpnConfigFileName());
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            QTextStream stream(&file);
            stream << openVpnConfigData << endl;
            file.close();
        }
        else {
            return ErrorCode::FailedToSaveConfigData;
        }
    }

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        QJsonObject ssConfigData = QJsonDocument::fromJson(
            createVpnConfigurationForProto(
                serverIndex, credentials, container, containerConfig, Protocol::ShadowSocks, &errorCode).toUtf8()).
            object();

        m_vpnConfiguration.insert(config::key_shadowsocks_config_data, ssConfigData);
    }

    if (container == DockerContainer::OpenVpnOverCloak) {
        QJsonObject cloakConfigData = QJsonDocument::fromJson(
            createVpnConfigurationForProto(
                serverIndex, credentials, container, containerConfig, Protocol::Cloak, &errorCode).toUtf8()).
            object();

        m_vpnConfiguration.insert(config::key_cloak_config_data, cloakConfigData);
    }

    //qDebug().noquote() << "VPN config" << QJsonDocument(m_vpnConfiguration).toJson();
    return ErrorCode::NoError;
}

ErrorCode VpnConnection::connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    qDebug() << "СonnectToVpn, CustomRouting is" << m_settings.customRouting();

    emit connectionStateChanged(VpnProtocol::ConnectionState::Connecting);

    if (m_vpnProtocol) {
        disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        m_vpnProtocol->stop();
        m_vpnProtocol.reset();
    }

    if (container == DockerContainer::None || container == DockerContainer::OpenVpn) {
        ErrorCode e = createVpnConfiguration(serverIndex, credentials, DockerContainer::OpenVpn, containerConfig);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }

        m_vpnProtocol.reset(new OpenVpnProtocol(m_vpnConfiguration));
        e = static_cast<OpenVpnProtocol *>(m_vpnProtocol.data())->checkAndSetupTapDriver();
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
    }
    else if (container == DockerContainer::OpenVpnOverShadowSocks) {
        ErrorCode e = createVpnConfiguration(serverIndex, credentials, DockerContainer::OpenVpnOverShadowSocks, containerConfig);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }

        m_vpnProtocol.reset(new ShadowSocksVpnProtocol(m_vpnConfiguration));
        e = static_cast<OpenVpnProtocol *>(m_vpnProtocol.data())->checkAndSetupTapDriver();
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
    }
    else if (container == DockerContainer::OpenVpnOverCloak) {
        ErrorCode e = createVpnConfiguration(serverIndex, credentials, DockerContainer::OpenVpnOverCloak, containerConfig);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }

        m_vpnProtocol.reset(new OpenVpnOverCloakProtocol(m_vpnConfiguration));
        e = static_cast<OpenVpnProtocol *>(m_vpnProtocol.data())->checkAndSetupTapDriver();
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
    }

    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

    ServerController::disconnectFromHost(credentials);

    return m_vpnProtocol.data()->start();
}

QString VpnConnection::bytesPerSecToText(quint64 bytes)
{
    double mbps = bytes * 8 / 1e6;
    return QString("%1 %2").arg(QString::number(mbps, 'f', 2)).arg(tr("Mbps")); // Mbit/s
}

void VpnConnection::disconnectFromVpn()
{
    qDebug() << "Disconnect from VPN";

    IpcClient::Interface()->flushDns();

    if (m_settings.customRouting()) {
        IpcClient::Interface()->clearSavedRoutes();
    }

    if (!m_vpnProtocol.data()) {
        return;
    }
    m_vpnProtocol.data()->stop();
}

VpnProtocol::ConnectionState VpnConnection::connectionState()
{
    if (!m_vpnProtocol) return VpnProtocol::ConnectionState::Disconnected;
    return m_vpnProtocol->connectionState();
}

bool VpnConnection::isConnected() const
{
    if (!m_vpnProtocol.data()) {
        return false;
    }

    return m_vpnProtocol.data()->isConnected();
}

bool VpnConnection::isDisconnected() const
{
    if (!m_vpnProtocol.data()) {
        return true;
    }

    return m_vpnProtocol.data()->isDisconnected();
}
