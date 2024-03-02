#include "ios_controller.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "../protocols/vpnprotocol.h"
#import "ios_controller_wrapper.h"

const char* Action::start = "start";
const char* Action::restart = "restart";
const char* Action::stop = "stop";
const char* Action::getTunnelId = "getTunnelId";
const char* Action::getStatus = "status";

const char* MessageKey::action = "action";
const char* MessageKey::tunnelId = "tunnelId";
const char* MessageKey::config = "config";
const char* MessageKey::errorCode = "errorCode";
const char* MessageKey::host = "host";
const char* MessageKey::port = "port";
const char* MessageKey::isOnDemand = "is-on-demand";
const char* MessageKey::SplitTunnelType = "SplitTunnelType";
const char* MessageKey::SplitTunnelSites = "SplitTunnelSites";


Vpn::ConnectionState iosStatusToState(NEVPNStatus status) {
  switch (status) {
    case NEVPNStatusInvalid:
        return Vpn::ConnectionState::Unknown;
    case NEVPNStatusDisconnected:
        return Vpn::ConnectionState::Disconnected;
    case NEVPNStatusConnecting:
        return Vpn::ConnectionState::Connecting;
    case NEVPNStatusConnected:
        return Vpn::ConnectionState::Connected;
    case NEVPNStatusReasserting:
        return Vpn::ConnectionState::Connecting;
    case NEVPNStatusDisconnecting:
        return Vpn::ConnectionState::Disconnecting;
    default:
        return Vpn::ConnectionState::Unknown;
}
}

namespace {
IosController* s_instance = nullptr;
}

IosController::IosController() : QObject()
{
    s_instance = this;
    m_iosControllerWrapper = [[IosControllerWrapper alloc] initWithCppController:this];

    [[NSNotificationCenter defaultCenter]
        removeObserver: (__bridge NSObject *)m_iosControllerWrapper];
    [[NSNotificationCenter defaultCenter]
        addObserver: (__bridge NSObject *)m_iosControllerWrapper selector:@selector(vpnStatusDidChange:) name:NEVPNStatusDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver: (__bridge NSObject *)m_iosControllerWrapper selector:@selector(vpnConfigurationDidChange:) name:NEVPNConfigurationChangeNotification object:nil];

}

IosController* IosController::Instance() {
    if (!s_instance) {
        s_instance = new IosController();
    }

    return s_instance;
}

bool IosController::initialize()
{
    __block bool ok = true;
    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> * _Nullable managers, NSError * _Nullable error) {
        @try {
            if (error) {
                qDebug() << "IosController::initialize : Error:" << [error.localizedDescription UTF8String];
                emit connectionStateChanged(Vpn::ConnectionState::Error);
                ok = false;
                return;
            }

            NSInteger managerCount = managers.count;
            qDebug() << "IosController::initialize : We have received managers:" << (long)managerCount;


            for (NETunnelProviderManager *manager in managers) {
                if (manager.connection.status == NEVPNStatusConnected) {
                    m_currentTunnel = manager;
                    qDebug() << "IosController::initialize : VPN already connected";
                    emit connectionStateChanged(Vpn::ConnectionState::Connected);
                    break;

                    // TODO: show connected state
                }
            }
        }
        @catch (NSException *exception) {
            qDebug() << "IosController::setTunnel : exception" << QString::fromNSString(exception.reason);
            ok = false;
        }
    }];

    return ok;
}

bool IosController::connectVpn(amnezia::Proto proto, const QJsonObject& configuration)
{
    m_proto = proto;
    m_rawConfig = configuration;
    m_serverAddress = configuration.value(config_key::hostName).toString().toNSString();

    QString tunnelName;
    if (configuration.value(config_key::description).toString().isEmpty()) {
        tunnelName = QString("%1 %2")
          .arg(configuration.value(config_key::hostName).toString())
          .arg(ProtocolProps::protoToString(proto));
    }
    else {
        tunnelName = QString("%1 (%2) %3")
          .arg(configuration.value(config_key::description).toString())
          .arg(configuration.value(config_key::hostName).toString())
          .arg(ProtocolProps::protoToString(proto));
    }

    qDebug() << "IosController::connectVpn" << tunnelName;

    m_currentTunnel = nullptr;

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block bool ok = true;
    __block bool isNewTunnelCreated = false;

    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> * _Nullable managers, NSError * _Nullable error) {
        @try {
            if (error) {
                qDebug() << "IosController::connectVpn : Error:" << [error.localizedDescription UTF8String];
                emit connectionStateChanged(Vpn::ConnectionState::Error);
                ok = false;
                return;
            }

            NSInteger managerCount = managers.count;
            qDebug() << "IosController::connectVpn : We have received managers:" << (long)managerCount;


            for (NETunnelProviderManager *manager in managers) {
                if ([manager.localizedDescription isEqualToString:tunnelName.toNSString()]) {
                    m_currentTunnel = manager;
                    qDebug() << "IosController::connectVpn : Using existing tunnel";
                    if (manager.connection.status == NEVPNStatusConnected) {
                        emit connectionStateChanged(Vpn::ConnectionState::Connected);
                        return;
                    }

                    break;
                }
            }

            if (!m_currentTunnel) {
                qDebug() << "IosController::connectVpn : Creating new tunnel";
                isNewTunnelCreated = true;
                m_currentTunnel = [[NETunnelProviderManager alloc] init];
                m_currentTunnel.localizedDescription = [NSString stringWithUTF8String:tunnelName.toStdString().c_str()];
            }

        }
        @catch (NSException *exception) {
            qDebug() << "IosController::connectVpn : exception" << QString::fromNSString(exception.reason);
            ok = false;
            m_currentTunnel = nullptr;
        }
        @finally {
            dispatch_semaphore_signal(semaphore);
        }
    }];

    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    if (!ok) return false;

    [[NSNotificationCenter defaultCenter]
        removeObserver:(__bridge NSObject *)m_iosControllerWrapper];

    [[NSNotificationCenter defaultCenter]
        addObserver:(__bridge NSObject *)m_iosControllerWrapper
            selector:@selector(vpnStatusDidChange:)
            name:NEVPNStatusDidChangeNotification
            object:m_currentTunnel.connection];


    if (proto == amnezia::Proto::OpenVpn) {
        return setupOpenVPN();
    }
    if (proto == amnezia::Proto::Cloak) {
        return setupCloak();
    }
    if (proto == amnezia::Proto::WireGuard) {
        return setupWireGuard();
    }
    if (proto == amnezia::Proto::Awg) {
        return setupAwg();
    }

    return false;
}

void IosController::disconnectVpn()
{
    if (!m_currentTunnel) {
        return;
    }

    if ([m_currentTunnel.connection isKindOfClass:[NETunnelProviderSession class]]) {
        [(NETunnelProviderSession *)m_currentTunnel.connection stopTunnel];
    }
}


void IosController::checkStatus()
{
    NSString *actionKey = [NSString stringWithUTF8String:MessageKey::action];
    NSString *actionValue = [NSString stringWithUTF8String:Action::getStatus];
    NSString *tunnelIdKey = [NSString stringWithUTF8String:MessageKey::tunnelId];
    NSString *tunnelIdValue = !m_tunnelId.isEmpty() ? m_tunnelId.toNSString() : @"";

    NSDictionary* message = @{actionKey: actionValue, tunnelIdKey: tunnelIdValue};
    sendVpnExtensionMessage(message, [&](NSDictionary* response){
        uint64_t txBytes = [response[@"tx_bytes"] intValue];
        uint64_t rxBytes = [response[@"rx_bytes"] intValue];
        emit bytesChanged(rxBytes - m_rxBytes, txBytes - m_txBytes);
        m_rxBytes = rxBytes;
        m_txBytes = txBytes;
    });
}

void IosController::vpnStatusDidChange(void *pNotification)
{
    NETunnelProviderSession *session = (NETunnelProviderSession *)pNotification;

    if (session /* && session == TunnelManager.session */ ) {
        qDebug() << "IosController::vpnStatusDidChange" << iosStatusToState(session.status) << session;

        if (session.status == NEVPNStatusDisconnected) {
            if (@available(iOS 16.0, *)) {
                [session fetchLastDisconnectErrorWithCompletionHandler:^(NSError * _Nullable error) {
                    if (error != nil) {
                        qDebug() << "Disconnect error" << error.domain << error.code << error.localizedDescription;

                        if ([error.domain isEqualToString:NEVPNConnectionErrorDomain]) {
                            switch (error.code) {
                                case NEVPNConnectionErrorOverslept:
                                    qDebug() << "Disconnect error info" << "The VPN connection was terminated because the system slept for an extended period of time.";
                                    break;
                                case NEVPNConnectionErrorNoNetworkAvailable:
                                    qDebug() << "Disconnect error info" << "The VPN connection could not be established because the system is not connected to a network.";
                                    break;
                                case NEVPNConnectionErrorUnrecoverableNetworkChange:
                                    qDebug() << "Disconnect error info" << "The VPN connection was terminated because the network conditions changed in such a way that the VPN connection could not be maintained.";
                                    break;
                                case NEVPNConnectionErrorConfigurationFailed:
                                    qDebug() << "Disconnect error info" << "The VPN connection could not be established because the configuration is invalid. ";
                                    break;
                                case NEVPNConnectionErrorServerAddressResolutionFailed:
                                    qDebug() << "Disconnect error info" << "The address of the VPN server could not be determined.";
                                    break;
                                case NEVPNConnectionErrorServerNotResponding:
                                    qDebug() << "Disconnect error info" << "Network communication with the VPN server has failed.";
                                    break;
                                case NEVPNConnectionErrorServerDead:
                                    qDebug() << "Disconnect error info" << "The VPN server is no longer functioning.";
                                    break;
                                case NEVPNConnectionErrorAuthenticationFailed:
                                    qDebug() << "Disconnect error info" << "The user credentials were rejected by the VPN server.";
                                    break;
                                case NEVPNConnectionErrorClientCertificateInvalid:
                                    qDebug() << "Disconnect error info" << "The client certificate is invalid.";
                                    break;
                                case NEVPNConnectionErrorClientCertificateNotYetValid:
                                    qDebug() << "Disconnect error info" << "The client certificate will not be valid until some future point in time.";
                                    break;
                                case NEVPNConnectionErrorClientCertificateExpired:
                                    qDebug() << "Disconnect error info" << "The validity period of the client certificate has passed.";
                                    break;
                                case NEVPNConnectionErrorPluginFailed:
                                    qDebug() << "Disconnect error info" << "The VPN plugin died unexpectedly.";
                                    break;
                                case NEVPNConnectionErrorConfigurationNotFound:
                                    qDebug() << "Disconnect error info" << "The VPN configuration could not be found.";
                                    break;
                                case NEVPNConnectionErrorPluginDisabled:
                                    qDebug() << "Disconnect error info" << "The VPN plugin could not be found or needed to be updated.";
                                    break;
                                case NEVPNConnectionErrorNegotiationFailed:
                                    qDebug() << "Disconnect error info" << "The VPN protocol negotiation failed.";
                                    break;
                                case NEVPNConnectionErrorServerDisconnected:
                                    qDebug() << "Disconnect error info" << "The VPN server terminated the connection.";
                                    break;
                                case NEVPNConnectionErrorServerCertificateInvalid:
                                    qDebug() << "Disconnect error info" << "The server certificate is invalid.";
                                    break;
                                case NEVPNConnectionErrorServerCertificateNotYetValid:
                                    qDebug() << "Disconnect error info" << "The server certificate will not be valid until some future point in time.";
                                    break;
                                case NEVPNConnectionErrorServerCertificateExpired:
                                    qDebug() << "Disconnect error info" << "The validity period of the server certificate has passed.";
                                    break;
                                default:
                                    qDebug() << "Disconnect error info" << "Unknown code.";
                                    break;
                            }
                        }

                        NSError *underlyingError = error.userInfo[@"NSUnderlyingError"];
                        if (underlyingError != nil) {
                            qDebug() << "Disconnect underlying error" << underlyingError.domain << underlyingError.code << underlyingError.localizedDescription;

                            if ([underlyingError.domain isEqualToString:@"NEAgentErrorDomain"]) {
                                switch (underlyingError.code) {
                                    case 1:
                                        qDebug() << "Disconnect underlying error" << "General. Use sysdiagnose.";
                                        break;
                                    case 2:
                                        qDebug() << "Disconnect underlying error" << "Plug-in unavailable. Use sysdiagnose.";
                                        break;
                                    default:
                                        qDebug() << "Disconnect underlying error" << "Unknown code. Use sysdiagnose.";
                                        break;
                                }
                            }
                        }
                    } else {
                        qDebug() << "Disconnect error is absent";
                    }
                }];
            } else {
                qDebug() << "Disconnect error is unavailable on iOS < 16.0";
            }
        }

        emit connectionStateChanged(iosStatusToState(session.status));
    }
}

void IosController::vpnConfigurationDidChange(void *pNotification)
{
    qDebug() << "IosController::vpnConfigurationDidChange" << pNotification;
}

bool IosController::setupOpenVPN()
{
    QJsonObject ovpn = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::OpenVpn)].toObject();
    QString ovpnConfig = ovpn[config_key::config].toString();

    QJsonObject openVPNConfig {};
    openVPNConfig.insert(config_key::config, ovpnConfig);
    openVPNConfig.insert(config_key::splitTunnelType, m_rawConfig[config_key::splitTunnelType]);

    QJsonArray splitTunnelSites = m_rawConfig[config_key::splitTunnelSites].toArray();

    for(int index = 0; index < splitTunnelSites.count(); index++) {
        splitTunnelSites[index] = splitTunnelSites[index].toString().remove(" ");
    }

    openVPNConfig.insert(config_key::splitTunnelSites, splitTunnelSites);

    QJsonDocument openVPNConfigDoc(openVPNConfig);
    QString openVPNConfigStr(openVPNConfigDoc.toJson(QJsonDocument::Compact));

    return startOpenVPN(openVPNConfigStr);
}

bool IosController::setupCloak()
{
    m_serverAddress = @"127.0.0.1";
    QJsonObject ovpn = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::OpenVpn)].toObject();
    QString ovpnConfig = ovpn[config_key::config].toString();

    QJsonObject cloak = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::Cloak)].toObject();

    cloak["NumConn"] = 1;
    if (cloak.contains("remote")) {
        cloak["RemoteHost"] = cloak["remote"].toString();
     }
    if (cloak.contains("port")) {
        cloak["RemotePort"] = cloak["port"].toString();
    }
    cloak.remove("remote");
    cloak.remove("port");
    cloak.remove("transport_proto");

    QJsonObject jsonObject {};
    foreach(const QString& key, cloak.keys()) {
        if(key == "NumConn" or key == "StreamTimeout"){
            jsonObject.insert(key, cloak.value(key).toInt());
        }else{
            jsonObject.insert(key, cloak.value(key).toString());
        }
    }
    QJsonDocument doc(jsonObject);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    QString cloakBase64 = strJson.toUtf8().toBase64();
    ovpnConfig.append("\n<cloak>\n");
    ovpnConfig.append(cloakBase64);
    ovpnConfig.append("\n</cloak>\n");

    QJsonObject openVPNConfig {};
    openVPNConfig.insert(config_key::config, ovpnConfig);
    openVPNConfig.insert(config_key::splitTunnelType, m_rawConfig[config_key::splitTunnelType]);

    QJsonArray splitTunnelSites = m_rawConfig[config_key::splitTunnelSites].toArray();

    for(int index = 0; index < splitTunnelSites.count(); index++) {
        splitTunnelSites[index] = splitTunnelSites[index].toString().remove(" ");
    }

    openVPNConfig.insert(config_key::splitTunnelSites, splitTunnelSites);

    QJsonDocument openVPNConfigDoc(openVPNConfig);
    QString openVPNConfigStr(openVPNConfigDoc.toJson(QJsonDocument::Compact));

    return startOpenVPN(openVPNConfigStr);
}

bool IosController::setupWireGuard()
{
    QJsonObject config = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::WireGuard)].toObject();

    QJsonObject wgConfig {};
    wgConfig.insert(config_key::dns1, m_rawConfig[config_key::dns1]);
    wgConfig.insert(config_key::dns2, m_rawConfig[config_key::dns2]);
    wgConfig.insert(config_key::hostName, config[config_key::hostName]);
    wgConfig.insert(config_key::port, config[config_key::port]);
    wgConfig.insert(config_key::client_ip, config[config_key::client_ip]);
    wgConfig.insert(config_key::client_priv_key, config[config_key::client_priv_key]);
    wgConfig.insert(config_key::server_pub_key, config[config_key::server_pub_key]);
    wgConfig.insert(config_key::psk_key, config[config_key::psk_key]);
    wgConfig.insert(config_key::splitTunnelType, m_rawConfig[config_key::splitTunnelType]);

    QJsonArray splitTunnelSites = m_rawConfig[config_key::splitTunnelSites].toArray();

    for(int index = 0; index < splitTunnelSites.count(); index++) {
        splitTunnelSites[index] = splitTunnelSites[index].toString().remove(" ");
    }

    wgConfig.insert(config_key::splitTunnelSites, splitTunnelSites);

    if (config.contains(config_key::allowed_ips)) {
        QJsonArray allowed_ips;
        QStringList allowed_ips_list = config[config_key::allowed_ips].toString().split(", ");

        for(int index = 0; index < allowed_ips_list.length(); index++) {
            allowed_ips.append(allowed_ips_list[index]);
        }

        wgConfig.insert(config_key::allowed_ips, allowed_ips);
    } else {
        QJsonArray allowed_ips { "0.0.0.0/0", "::/0" };
        wgConfig.insert(config_key::allowed_ips, allowed_ips);
    }

    wgConfig.insert("persistent_keep_alive", "25");

    QJsonDocument wgConfigDoc(wgConfig);
    QString wgConfigDocStr(wgConfigDoc.toJson(QJsonDocument::Compact));

    return startWireGuard(wgConfigDocStr);
}

bool IosController::setupAwg()
{
    QJsonObject config = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::Awg)].toObject();

    QJsonObject wgConfig {};
    wgConfig.insert(config_key::dns1, m_rawConfig[config_key::dns1]);
    wgConfig.insert(config_key::dns2, m_rawConfig[config_key::dns2]);
    wgConfig.insert(config_key::hostName, config[config_key::hostName]);
    wgConfig.insert(config_key::port, config[config_key::port]);
    wgConfig.insert(config_key::client_ip, config[config_key::client_ip]);
    wgConfig.insert(config_key::client_priv_key, config[config_key::client_priv_key]);
    wgConfig.insert(config_key::server_pub_key, config[config_key::server_pub_key]);
    wgConfig.insert(config_key::psk_key, config[config_key::psk_key]);
    wgConfig.insert(config_key::splitTunnelType, m_rawConfig[config_key::splitTunnelType]);

    QJsonArray splitTunnelSites = m_rawConfig[config_key::splitTunnelSites].toArray();

    for(int index = 0; index < splitTunnelSites.count(); index++) {
        splitTunnelSites[index] = splitTunnelSites[index].toString().remove(" ");
    }

    wgConfig.insert(config_key::splitTunnelSites, splitTunnelSites);

    if (config.contains(config_key::allowed_ips)) {
        QJsonArray allowed_ips;
        QStringList allowed_ips_list = config[config_key::allowed_ips].toString().split(", ");

        for(int index = 0; index < allowed_ips_list.length(); index++) {
            allowed_ips.append(allowed_ips_list[index]);
        }

        wgConfig.insert(config_key::allowed_ips, allowed_ips);
    } else {
        QJsonArray allowed_ips { "0.0.0.0/0", "::/0" };
        wgConfig.insert(config_key::allowed_ips, allowed_ips);
    }

    wgConfig.insert("persistent_keep_alive", "25");
    wgConfig.insert(config_key::initPacketMagicHeader, config[config_key::initPacketMagicHeader]);
    wgConfig.insert(config_key::responsePacketMagicHeader, config[config_key::responsePacketMagicHeader]);
    wgConfig.insert(config_key::underloadPacketMagicHeader, config[config_key::underloadPacketMagicHeader]);
    wgConfig.insert(config_key::transportPacketMagicHeader, config[config_key::transportPacketMagicHeader]);

    wgConfig.insert(config_key::initPacketJunkSize, config[config_key::initPacketJunkSize]);
    wgConfig.insert(config_key::responsePacketJunkSize, config[config_key::responsePacketJunkSize]);

    wgConfig.insert(config_key::junkPacketCount, config[config_key::junkPacketCount]);
    wgConfig.insert(config_key::junkPacketMinSize, config[config_key::junkPacketMinSize]);
    wgConfig.insert(config_key::junkPacketMaxSize, config[config_key::junkPacketMaxSize]);

    QJsonDocument wgConfigDoc(wgConfig);
    QString wgConfigDocStr(wgConfigDoc.toJson(QJsonDocument::Compact));

    return startWireGuard(wgConfigDocStr);
}

bool IosController::startOpenVPN(const QString &config)
{
    qDebug() << "IosController::startOpenVPN";

    NETunnelProviderProtocol *tunnelProtocol = [[NETunnelProviderProtocol alloc] init];
    tunnelProtocol.providerBundleIdentifier = [NSString stringWithUTF8String:VPN_NE_BUNDLEID];
    tunnelProtocol.providerConfiguration = @{@"ovpn": [[NSString stringWithUTF8String:config.toStdString().c_str()] dataUsingEncoding:NSUTF8StringEncoding]};
    tunnelProtocol.serverAddress = m_serverAddress;

    m_currentTunnel.protocolConfiguration = tunnelProtocol;

    startTunnel();
}

bool IosController::startWireGuard(const QString &config)
{
    qDebug() << "IosController::startWireGuard";

    NETunnelProviderProtocol *tunnelProtocol = [[NETunnelProviderProtocol alloc] init];
    tunnelProtocol.providerBundleIdentifier = [NSString stringWithUTF8String:VPN_NE_BUNDLEID];
    tunnelProtocol.providerConfiguration = @{@"wireguard": [[NSString stringWithUTF8String:config.toStdString().c_str()] dataUsingEncoding:NSUTF8StringEncoding]};
    tunnelProtocol.serverAddress = m_serverAddress;

    m_currentTunnel.protocolConfiguration = tunnelProtocol;

    startTunnel();
}

void IosController::startTunnel()
{
    NSString *protocolName = @"Unknown";

    NETunnelProviderProtocol *tunnelProtocol = (NETunnelProviderProtocol *)m_currentTunnel.protocolConfiguration;
    if (tunnelProtocol.providerConfiguration[@"wireguard"] != nil) {
        protocolName = @"WireGuard";
    } else if (tunnelProtocol.providerConfiguration[@"ovpn"] != nil) {
        protocolName = @"OpenVPN";
    }

    m_rxBytes = 0;
    m_txBytes = 0;

    [m_currentTunnel setEnabled:YES];

    [m_currentTunnel saveToPreferencesWithCompletionHandler:^(NSError *saveError) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

            if (saveError) {
                emit connectionStateChanged(Vpn::ConnectionState::Error);
                return;
            }

            [m_currentTunnel loadFromPreferencesWithCompletionHandler:^(NSError *loadError) {
                    if (loadError) {
                        qDebug().nospace() << "IosController::start" << protocolName << ": Connect " << protocolName << " Tunnel Load Error" << loadError.localizedDescription.UTF8String;
                        emit connectionStateChanged(Vpn::ConnectionState::Error);
                        return;
                    }

                    NSError *startError = nil;
                    qDebug() << iosStatusToState(m_currentTunnel.connection.status);

                    BOOL started = [m_currentTunnel.connection startVPNTunnelWithOptions:nil andReturnError:&startError];

                    if (!started || startError) {
                        qDebug().nospace() << "IosController::start" << protocolName << " : Connect " << protocolName << " Tunnel Start Error"
                            << (startError ? startError.localizedDescription.UTF8String : "");
                        emit connectionStateChanged(Vpn::ConnectionState::Error);
                    } else {
                        qDebug().nospace() << "IosController::start" << protocolName << " : Starting the tunnel succeeded";
                    }
            }];
        });
    }];
}

bool IosController::isOurManager(NETunnelProviderManager* manager) {
    NETunnelProviderProtocol* tunnelProto = (NETunnelProviderProtocol*)manager.protocolConfiguration;

    if (!tunnelProto) {
        qDebug() << "Ignoring manager because the proto is invalid";
        return false;
    }

    if (!tunnelProto.providerBundleIdentifier) {
        qDebug() << "Ignoring manager because the bundle identifier is null";
        return false;
    }

    if (![tunnelProto.providerBundleIdentifier isEqualToString:[NSString stringWithUTF8String:VPN_NE_BUNDLEID]]) {
        qDebug() << "Ignoring manager because the bundle identifier doesn't match";
        return false;
    }

    qDebug() << "Found the manager with the correct bundle identifier:" << QString::fromNSString(tunnelProto.providerBundleIdentifier);

    return true;
}

void IosController::sendVpnExtensionMessage(NSDictionary* message, std::function<void(NSDictionary*)> callback)
{
    if (!m_currentTunnel) {
        qDebug() << "Cannot set an extension callback without a tunnel manager";
        return;
    }

    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:message options:0 error:&error];

    if (!data || error) {
        qDebug() << "Failed to serialize message to VpnExtension as JSON. Error:"
                 << [error.localizedDescription UTF8String];
        return;
    }

    void (^completionHandler)(NSData *) = ^(NSData *responseData) {
        if (!responseData) {
            if (callback) callback(nil);
            return;
        }

        NSError *deserializeError = nil;
        NSDictionary *response = [NSJSONSerialization JSONObjectWithData:responseData options:0 error:&deserializeError];

        if (response && [response isKindOfClass:[NSDictionary class]]) {
            if (callback) callback(response);
            return;
        } else if (deserializeError) {
            qDebug() << "Failed to deserialize the VpnExtension response";
        }

        if (callback) callback(nil);
    };

    NETunnelProviderSession *session = (NETunnelProviderSession *)m_currentTunnel.connection;

    NSError *sendError = nil;

    if ([session respondsToSelector:@selector(sendProviderMessage:returnError:responseHandler:)]) {
        [session sendProviderMessage:data returnError:&sendError responseHandler:completionHandler];
    } else {
        qDebug() << "Method sendProviderMessage:responseHandler:error: does not exist";
    }

    if (sendError) {
        qDebug() << "Failed to send message to VpnExtension. Error:"
                 << [sendError.localizedDescription UTF8String];
    }

}
