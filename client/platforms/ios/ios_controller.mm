#include "ios_controller.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QEventLoop>

#include "../core/protocols/vpnProtocol.h"
#import "ios_controller_wrapper.h"
#import "core/utils/swiftBridge.h"

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

using namespace ProtocolUtils;

#if !MACOS_NE
static UIViewController* getViewController() {
    UIApplication *application = [UIApplication sharedApplication];

    if (@available(iOS 13.0, *)) {
        for (UIScene *scene in application.connectedScenes) {
            if (scene.activationState != UISceneActivationStateForegroundActive) {
                continue;
            }

            if (![scene isKindOfClass:[UIWindowScene class]]) {
                continue;
            }

            UIWindowScene *windowScene = (UIWindowScene *)scene;

            for (UIWindow *window in windowScene.windows) {
                if (window.isKeyWindow && window.rootViewController) {
                    return window.rootViewController;
                }
            }

            for (UIWindow *window in windowScene.windows) {
                if (!window.isHidden && window.rootViewController) {
                    return window.rootViewController;
                }
            }
        }
    }

    for (UIWindow *window in application.windows) {
        if (window.isKeyWindow && window.rootViewController) {
            return window.rootViewController;
        }
    }

    for (UIWindow *window in application.windows) {
        if (window.rootViewController) {
            return window.rootViewController;
        }
    }

    return nil;
}
#endif

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
constexpr int kHandshakeTimeoutMs = 12000;
constexpr uint64_t kHandshakeRxThreshold = 4096;
bool isWireGuardBasedProto(amnezia::Proto proto) {
    return proto == amnezia::Proto::WireGuard || proto == amnezia::Proto::Awg;
}

uint64_t uint64FromResponse(NSDictionary *response, NSString *key, uint64_t fallback = 0) {
    id value = response[key];
    if (!value || value == [NSNull null]) {
        return fallback;
    }
    if ([value isKindOfClass:[NSNumber class]]) {
        return [(NSNumber *)value unsignedLongLongValue];
    }
    if ([value isKindOfClass:[NSString class]]) {
        const char *str = [(NSString *)value UTF8String];
        if (str && *str) {
            return strtoull(str, nullptr, 10);
        }
    }
    return fallback;
}

long long int64FromResponse(NSDictionary *response, NSString *key, long long fallback = 0) {
    id value = response[key];
    if (!value || value == [NSNull null]) {
        return fallback;
    }
    if ([value isKindOfClass:[NSNumber class]]) {
        return [(NSNumber *)value longLongValue];
    }
    if ([value isKindOfClass:[NSString class]]) {
        const char *str = [(NSString *)value UTF8String];
        if (str && *str) {
            return strtoll(str, nullptr, 10);
        }
    }
    return fallback;
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

void IosController::emitConnectionStateIfChanged(Vpn::ConnectionState state)
{
    if (m_lastEmittedState == state) {
        return;
    }
    m_lastEmittedState = state;
    emit connectionStateChanged(state);
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
                qWarning() << "IosController::initialize : loadAllFromPreferences failed:"
                           << [error.localizedDescription UTF8String]
                           << "domain:" << [error.domain UTF8String] << "code:" << error.code;
                ok = false;
                return;
            }

            NSInteger managerCount = managers.count;
            qDebug() << "IosController::initialize : We have received managers:" << (long)managerCount;


            for (NETunnelProviderManager *manager in managers) {
                qDebug() << "IosController::initialize : VPNC: " << manager.localizedDescription;

                if (manager.connection.status == NEVPNStatusConnected) {
                    m_currentTunnel = manager;
                    qDebug() << "IosController::initialize : VPN already connected with" << manager.localizedDescription;
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
    m_serverAddress = configuration.value(configKey::hostName).toString().toNSString();

    const QString serverDescription = configuration.value(configKey::description).toString().trimmed();
    QString tunnelName;
    if (serverDescription.isEmpty()) {
        tunnelName = ProtocolUtils::protoToString(proto);
    } else {
        tunnelName = QString("%1 %2")
          .arg(serverDescription)
          .arg(ProtocolUtils::protoToString(proto));
    }

    qDebug() << "IosController::connectVpn" << tunnelName;

    m_currentTunnel = nullptr;

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block bool ok = true;
    __block bool isNewTunnelCreated = false;

    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> * _Nullable managers, NSError * _Nullable error) {
        @try {
            if (error) {
                qDebug() << "IosController::connectVpn : VPNC: loadAllFromPreferences error:" << [error.localizedDescription UTF8String];
                emit connectionStateChanged(Vpn::ConnectionState::Error);
                ok = false;
                return;
            }

            NSInteger managerCount = managers.count;
            qDebug() << "IosController::connectVpn : We have received managers:" << (long)managerCount;


            for (NETunnelProviderManager *manager in managers) {
                if ([manager.localizedDescription isEqualToString:tunnelName.toNSString()]) {
                    m_currentTunnel = manager;
                    qDebug() << "IosController::connectVpn : Using existing tunnel:" << manager.localizedDescription;
                    if (manager.connection.status == NEVPNStatusConnected) {
                        emit connectionStateChanged(Vpn::ConnectionState::Connected);
                        return;
                    }

                    break;
                }
            }

            if (!m_currentTunnel) {
                isNewTunnelCreated = true;
                m_currentTunnel = [[NETunnelProviderManager alloc] init];
                m_currentTunnel.localizedDescription = [NSString stringWithUTF8String:tunnelName.toStdString().c_str()];
                qDebug() << "IosController::connectVpn : Creating new tunnel" << m_currentTunnel.localizedDescription;
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
    if (proto == amnezia::Proto::WireGuard) {
        return setupWireGuard();
    }
    if (proto == amnezia::Proto::Awg) {
        return setupAwg();
    }
    if (proto == amnezia::Proto::Xray) {
        return setupXray();
    }
    if (proto == amnezia::Proto::SSXray) {
        return setupSSXray();
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
    if (!m_currentTunnel) {
        return;
    }

    if (m_currentTunnel.connection.status != NEVPNStatusConnected) {
        return;
    }

    if (m_statusRequestInFlight.exchange(true)) {
        return;
    }

    NSString *actionKey = [NSString stringWithUTF8String:MessageKey::action];
    NSString *actionValue = [NSString stringWithUTF8String:Action::getStatus];
    NSString *tunnelIdKey = [NSString stringWithUTF8String:MessageKey::tunnelId];
    NSString *tunnelIdValue = !m_tunnelId.isEmpty() ? m_tunnelId.toNSString() : @"";

    NSDictionary* message = @{actionKey: actionValue, tunnelIdKey: tunnelIdValue};
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    sendVpnExtensionMessage(message, [&](NSDictionary* response){
        if (!response) {
            QMetaObject::invokeMethod(this, [this]() {
                m_statusRequestInFlight = false;
            }, Qt::QueuedConnection);
            return;
        }

        const uint64_t txBytes = uint64FromResponse(response, @"tx_bytes");
        const uint64_t rxBytes = uint64FromResponse(response, @"rx_bytes");
        const long long last_handshake_time_sec = int64FromResponse(response, @"last_handshake_time_sec");

        QMetaObject::invokeMethod(this, [this, txBytes, rxBytes, last_handshake_time_sec]() {
            if (isWireGuardBasedProto(m_proto) && m_handshakeAwaiting) {
                const bool hasHandshakeData = (last_handshake_time_sec >= 0);
                const bool hasFreshHandshake = hasHandshakeData &&
                        ((last_handshake_time_sec > 0) ||
                         (rxBytes >= kHandshakeRxThreshold) ||
                         (txBytes >= kHandshakeRxThreshold));

                if (hasFreshHandshake) {
                    m_handshakeConfirmed = true;
                    m_handshakeAwaiting = false;
                    m_handshakeTimer.invalidate();
                    qDebug() << "IosController::checkStatus : handshake confirmed";
                    emitConnectionStateIfChanged(Vpn::ConnectionState::Connected);
                } else if (m_handshakeTimer.isValid() &&
                           m_handshakeTimer.elapsed() > kHandshakeTimeoutMs) {
                    m_handshakeTimer.restart();
                    qDebug() << "IosController::checkStatus : handshake timed out, keeping tunnel alive";
                    emitConnectionStateIfChanged(Vpn::ConnectionState::Reconnecting);
                }
            }

            emit bytesChanged(rxBytes - m_rxBytes, txBytes - m_txBytes);
            m_rxBytes = rxBytes;
            m_txBytes = txBytes;
            m_statusRequestInFlight = false;
        }, Qt::QueuedConnection);
    });
    });
}

void IosController::vpnStatusDidChange(void *pNotification)
{
    NETunnelProviderSession *session = (NETunnelProviderSession *)pNotification;

    if (!session) {
        return;
    }
    if (!m_currentTunnel || (NETunnelProviderSession *)m_currentTunnel.connection != session) {
        return;
    }

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
                    }
                }];
            } else {
                qDebug() << "Disconnect error is unavailable on iOS < 16.0";
            }
        }

        Vpn::ConnectionState nextState = iosStatusToState(session.status);
        if (session.status == NEVPNStatusConnected && isWireGuardBasedProto(m_proto)) {
            if (!m_handshakeConfirmed) {
                nextState = Vpn::ConnectionState::Connecting;
                if (!m_handshakeAwaiting) {
                    m_handshakeAwaiting = true;
                    m_handshakeTimer.restart();
                }
            }
        } else if (session.status != NEVPNStatusConnected) {
            m_handshakeAwaiting = false;
            m_handshakeConfirmed = false;
            m_handshakeTimer.invalidate();
            m_statusRequestInFlight = false;
        }
        emitConnectionStateIfChanged(nextState);
}

void IosController::vpnConfigurationDidChange(void *pNotification)
{
    qDebug() << "IosController::vpnConfigurationDidChange" << pNotification;
}

bool IosController::setupOpenVPN()
{
    QJsonObject ovpn = m_rawConfig[ProtocolUtils::key_proto_config_data(amnezia::Proto::OpenVpn)].toObject();
    QString ovpnConfig = ovpn[configKey::config].toString();

    QJsonObject openVPNConfig {};
    openVPNConfig.insert(configKey::config, ovpnConfig);

    if (ovpn.contains(configKey::mtu)) {
        openVPNConfig.insert(configKey::mtu, ovpn[configKey::mtu]);
    } else {
        openVPNConfig.insert(configKey::mtu, protocols::openvpn::defaultMtu);
    }

    openVPNConfig.insert(configKey::splitTunnelType, m_rawConfig[configKey::splitTunnelType]);

    QJsonArray splitTunnelSites = m_rawConfig[configKey::splitTunnelSites].toArray();

    for(int index = 0; index < splitTunnelSites.count(); index++) {
        splitTunnelSites[index] = splitTunnelSites[index].toString().remove(" ");
    }

    openVPNConfig.insert(configKey::splitTunnelSites, splitTunnelSites);

    QJsonDocument openVPNConfigDoc(openVPNConfig);
    QString openVPNConfigStr(openVPNConfigDoc.toJson(QJsonDocument::Compact));

    return startOpenVPN(openVPNConfigStr);
}

static void insertNonEmptyAwgParams(QJsonObject &wgConfig, const QJsonObject &config)
{
    const QStringList awgProtocolKeys = configKey::awgProtocolKeys();

    for (const QString &key : awgProtocolKeys) {
        const QJsonValue value = config.value(key);
        if (value.isString() && !value.toString().isEmpty()) {
            wgConfig.insert(key, value);
        }
    }
}

bool IosController::setupWireGuard()
{
    QJsonObject config = m_rawConfig[ProtocolUtils::key_proto_config_data(amnezia::Proto::WireGuard)].toObject();

    QJsonObject wgConfig {};
    wgConfig.insert(configKey::dns1, m_rawConfig[configKey::dns1]);
    wgConfig.insert(configKey::dns2, m_rawConfig[configKey::dns2]);

    if (config.contains(configKey::mtu)) {
        wgConfig.insert(configKey::mtu, config[configKey::mtu]);
    } else {
        wgConfig.insert(configKey::mtu, protocols::wireguard::defaultMtu);
    }

    wgConfig.insert(configKey::hostName, config[configKey::hostName]);
    wgConfig.insert(configKey::port, config[configKey::port]);
    QStringList clientAddresses;
    for (const QString &address : config.value(configKey::clientIp).toString().split(",", Qt::SkipEmptyParts)) {
        clientAddresses << address.trimmed();
    }
    for (const QString &address : config.value(configKey::clientIpv6).toString().split(",", Qt::SkipEmptyParts)) {
        const QString trimmedAddress = address.trimmed();
        if (!clientAddresses.contains(trimmedAddress)) {
            clientAddresses << trimmedAddress;
        }
    }

    // Determine the routes (AllowedIPs). A full-tunnel config must also capture IPv6 (::/0),
    // even when the server has no IPv6, so native IPv6 cannot leak over the cellular/Wi-Fi
    // interface. Custom split/partial route sets are honored verbatim.
    QJsonArray allowedIps;
    if (config.contains(configKey::allowedIps) && config[configKey::allowedIps].isArray()) {
        allowedIps = config[configKey::allowedIps].toArray();
    } else {
        allowedIps = QJsonArray { "0.0.0.0/0" };
    }
    const bool isFullTunnel = allowedIps.contains("0.0.0.0/0");
    if (isFullTunnel && !allowedIps.contains("::/0")) {
        allowedIps.append("::/0");
    }

    bool hasRealClientIpv6 = false;
    for (const QString &address : clientAddresses) {
        if (address.contains(":")) {
            hasRealClientIpv6 = true;
            break;
        }
    }
    // For a full-tunnel config with no server-assigned IPv6, attach a non-routable ULA so the
    // ::/0 route installs and IPv6 is black-holed in the tunnel instead of leaking.
    if (isFullTunnel && !hasRealClientIpv6) {
        clientAddresses << QString("%1/%2").arg(protocols::wireguard::dummyClientIpv6Address,
                                                protocols::wireguard::defaultClientIpv6Cidr);
    }
    const bool hasClientIpv6 = isFullTunnel || hasRealClientIpv6;

    wgConfig.insert(configKey::clientIp, clientAddresses.join(", "));
    wgConfig.insert(configKey::clientPrivKey, config[configKey::clientPrivKey]);
    wgConfig.insert(configKey::serverPubKey, config[configKey::serverPubKey]);
    wgConfig.insert(configKey::pskKey, config[configKey::pskKey]);
    wgConfig.insert(configKey::splitTunnelType, m_rawConfig[configKey::splitTunnelType]);

    QJsonArray splitTunnelSites;
    const QJsonArray rawSplitTunnelSites = m_rawConfig[configKey::splitTunnelSites].toArray();
    for(int index = 0; index < rawSplitTunnelSites.count(); index++) {
        const QString site = rawSplitTunnelSites[index].toString().remove(" ");
        if (hasClientIpv6 || !site.contains(":")) {
            splitTunnelSites.append(site);
        }
    }

    wgConfig.insert(configKey::splitTunnelSites, splitTunnelSites);

    QJsonArray filteredAllowedIps;
    for (const QJsonValue &value : allowedIps) {
        const QString route = value.toString();
        if (hasClientIpv6 || !route.contains(":")) {
            filteredAllowedIps.append(route);
        }
    }
    wgConfig.insert(configKey::allowedIps, filteredAllowedIps);

    if (config.contains(configKey::persistentKeepAlive)) {
        wgConfig.insert(configKey::persistentKeepAlive, config[configKey::persistentKeepAlive]);
    }

    insertNonEmptyAwgParams(wgConfig, config);

    QJsonDocument wgConfigDoc(wgConfig);
    QString wgConfigDocStr(wgConfigDoc.toJson(QJsonDocument::Compact));

    return startWireGuard(wgConfigDocStr);
}

bool IosController::setupXray()
{
    QJsonObject config = m_rawConfig[ProtocolUtils::key_proto_config_data(amnezia::Proto::Xray)].toObject();
    QString xrayConfigStr = config.value(configKey::config).toString();

    QJsonObject finalConfig;
    finalConfig.insert(configKey::dns1, m_rawConfig[configKey::dns1].toString());
    finalConfig.insert(configKey::dns2, m_rawConfig[configKey::dns2].toString());
    finalConfig.insert(configKey::splitTunnelType, m_rawConfig[configKey::splitTunnelType]);

    QJsonArray splitTunnelSites = m_rawConfig[configKey::splitTunnelSites].toArray();

    for (int index = 0; index < splitTunnelSites.count(); index++) {
        splitTunnelSites[index] = splitTunnelSites[index].toString().remove(" ");
    }

    finalConfig.insert(configKey::splitTunnelSites, splitTunnelSites);
    finalConfig.insert(configKey::config, xrayConfigStr);

    QJsonDocument finalConfigDoc(finalConfig);
    QString finalConfigStr(finalConfigDoc.toJson(QJsonDocument::Compact));

    return startXray(finalConfigStr);
}

bool IosController::setupSSXray()
{
    QJsonObject config = m_rawConfig[ProtocolUtils::key_proto_config_data(amnezia::Proto::SSXray)].toObject();
    QString ssXrayConfigStr = config.value(configKey::config).toString();

    QJsonObject finalConfig;
    finalConfig.insert(configKey::dns1, m_rawConfig[configKey::dns1]);
    finalConfig.insert(configKey::dns2, m_rawConfig[configKey::dns2]);
    finalConfig.insert(configKey::config, ssXrayConfigStr);

    QJsonDocument finalConfigDoc(finalConfig);
    QString finalConfigStr(finalConfigDoc.toJson(QJsonDocument::Compact));

    return startXray(finalConfigStr);
}

bool IosController::setupAwg()
{
    QJsonObject config = m_rawConfig[ProtocolUtils::key_proto_config_data(amnezia::Proto::Awg)].toObject();

    QJsonObject wgConfig {};
    wgConfig.insert(configKey::dns1, m_rawConfig[configKey::dns1]);
    wgConfig.insert(configKey::dns2, m_rawConfig[configKey::dns2]);

    if (config.contains(configKey::mtu)) {
        wgConfig.insert(configKey::mtu, config[configKey::mtu]);
    } else {
        wgConfig.insert(configKey::mtu, protocols::awg::defaultMtu);
    }

    wgConfig.insert(configKey::hostName, config[configKey::hostName]);
    wgConfig.insert(configKey::port, config[configKey::port]);
    QStringList clientAddresses;
    for (const QString &address : config.value(configKey::clientIp).toString().split(",", Qt::SkipEmptyParts)) {
        clientAddresses << address.trimmed();
    }
    for (const QString &address : config.value(configKey::clientIpv6).toString().split(",", Qt::SkipEmptyParts)) {
        const QString trimmedAddress = address.trimmed();
        if (!clientAddresses.contains(trimmedAddress)) {
            clientAddresses << trimmedAddress;
        }
    }

    // Determine the routes (AllowedIPs). A full-tunnel config must also capture IPv6 (::/0),
    // even when the server has no IPv6, so native IPv6 cannot leak over the cellular/Wi-Fi
    // interface. Custom split/partial route sets are honored verbatim.
    QJsonArray allowedIps;
    if (config.contains(configKey::allowedIps) && config[configKey::allowedIps].isArray()) {
        allowedIps = config[configKey::allowedIps].toArray();
    } else {
        allowedIps = QJsonArray { "0.0.0.0/0" };
    }
    const bool isFullTunnel = allowedIps.contains("0.0.0.0/0");
    if (isFullTunnel && !allowedIps.contains("::/0")) {
        allowedIps.append("::/0");
    }

    bool hasRealClientIpv6 = false;
    for (const QString &address : clientAddresses) {
        if (address.contains(":")) {
            hasRealClientIpv6 = true;
            break;
        }
    }
    // For a full-tunnel config with no server-assigned IPv6, attach a non-routable ULA so the
    // ::/0 route installs and IPv6 is black-holed in the tunnel instead of leaking.
    if (isFullTunnel && !hasRealClientIpv6) {
        clientAddresses << QString("%1/%2").arg(protocols::wireguard::dummyClientIpv6Address,
                                                protocols::wireguard::defaultClientIpv6Cidr);
    }
    const bool hasClientIpv6 = isFullTunnel || hasRealClientIpv6;

    wgConfig.insert(configKey::clientIp, clientAddresses.join(", "));
    wgConfig.insert(configKey::clientPrivKey, config[configKey::clientPrivKey]);
    wgConfig.insert(configKey::serverPubKey, config[configKey::serverPubKey]);
    wgConfig.insert(configKey::pskKey, config[configKey::pskKey]);
    wgConfig.insert(configKey::splitTunnelType, m_rawConfig[configKey::splitTunnelType]);

    QJsonArray splitTunnelSites;
    const QJsonArray rawSplitTunnelSites = m_rawConfig[configKey::splitTunnelSites].toArray();
    for(int index = 0; index < rawSplitTunnelSites.count(); index++) {
        const QString site = rawSplitTunnelSites[index].toString().remove(" ");
        if (hasClientIpv6 || !site.contains(":")) {
            splitTunnelSites.append(site);
        }
    }

    wgConfig.insert(configKey::splitTunnelSites, splitTunnelSites);

    QJsonArray filteredAllowedIps;
    for (const QJsonValue &value : allowedIps) {
        const QString route = value.toString();
        if (hasClientIpv6 || !route.contains(":")) {
            filteredAllowedIps.append(route);
        }
    }
    wgConfig.insert(configKey::allowedIps, filteredAllowedIps);

    if (config.contains(configKey::persistentKeepAlive)) {
        wgConfig.insert(configKey::persistentKeepAlive, config[configKey::persistentKeepAlive]);
    }

    insertNonEmptyAwgParams(wgConfig, config);

    QJsonDocument wgConfigDoc(wgConfig);
    QString wgConfigDocStr(wgConfigDoc.toJson(QJsonDocument::Compact));

    return startWireGuard(wgConfigDocStr);
}

bool IosController::startOpenVPN(const QString &config)
{
    qDebug() << "IosController::startOpenVPN";

    NETunnelProviderProtocol *tunnelProtocol = [[NETunnelProviderProtocol alloc] init];
    tunnelProtocol.providerBundleIdentifier = [NSString stringWithUTF8String:VPN_NE_BUNDLEID];
    QByteArray configUtf8 = config.toUtf8();
    NSData *ovpnConfigData = [NSData dataWithBytes:configUtf8.constData() length:configUtf8.size()];
    tunnelProtocol.providerConfiguration = @{@"ovpn": ovpnConfigData};
    tunnelProtocol.serverAddress = m_serverAddress;
    if (@available(iOS 14.0, macOS 11.0, *)) {
        int splitTunnelType = 0;
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            splitTunnelType = obj.value(configKey::splitTunnelType).toInt(0);
        }
#if defined(MACOS_NE)
        // On macOS NE use route-based full tunnel. includeAllNetworks enables
        // policy-based drop-all mode and causes enforceRoutes to be ignored.
        tunnelProtocol.includeAllNetworks = NO;
        if (splitTunnelType == 0) {
            tunnelProtocol.enforceRoutes = YES;
            if (@available(iOS 14.2, macOS 11.0, *)) {
                tunnelProtocol.excludeLocalNetworks = YES;
            }
        }
#else
        tunnelProtocol.includeAllNetworks = (splitTunnelType == 0);
        if (@available(iOS 14.2, macOS 11.0, *)) {
            // Keep existing iOS behavior.
            if (splitTunnelType == 0) {
                tunnelProtocol.excludeLocalNetworks = NO;
            }
        }
#endif
    }

    m_currentTunnel.protocolConfiguration = tunnelProtocol;

    NETunnelProviderProtocol *appliedProtocol = (NETunnelProviderProtocol *)m_currentTunnel.protocolConfiguration;
    NSData *ovpnPayload = appliedProtocol.providerConfiguration[@"ovpn"];
    NSString *payloadPreview = @"";
    if (ovpnPayload != nil) {
        NSString *decodedPayload = [[NSString alloc] initWithData:ovpnPayload encoding:NSUTF8StringEncoding];
        if (decodedPayload != nil) {
            payloadPreview = [decodedPayload substringToIndex:MIN((NSUInteger)512, decodedPayload.length)];
        }
    }

    qDebug().noquote() << "IosController::startOpenVPN protocolConfiguration"
                       << "bundleId=" << QString::fromNSString(appliedProtocol.providerBundleIdentifier ?: @"")
                       << "serverAddress=" << QString::fromNSString(appliedProtocol.serverAddress ?: @"")
                       << "providerKeys=" << QString::fromNSString([[appliedProtocol.providerConfiguration.allKeys description] copy])
                       << "ovpnBytes=" << (ovpnPayload != nil ? ovpnPayload.length : 0);
    qDebug().noquote() << "IosController::startOpenVPN protocolConfiguration payloadPreview="
                       << QString::fromNSString(payloadPreview);

    startTunnel();
}

bool IosController::startWireGuard(const QString &config)
{
    qDebug() << "IosController::startWireGuard";

    NETunnelProviderProtocol *tunnelProtocol = [[NETunnelProviderProtocol alloc] init];
    tunnelProtocol.providerBundleIdentifier = [NSString stringWithUTF8String:VPN_NE_BUNDLEID];
    QByteArray configUtf8 = config.toUtf8();
    NSData *wgConfigData = [NSData dataWithBytes:configUtf8.constData() length:configUtf8.size()];
    tunnelProtocol.providerConfiguration = @{@"wireguard": wgConfigData};
    tunnelProtocol.serverAddress = m_serverAddress;

    m_currentTunnel.protocolConfiguration = tunnelProtocol;

    startTunnel();
}

bool IosController::startXray(const QString &config)
{
    qDebug() << "IosController::startXray";

    NETunnelProviderProtocol *tunnelProtocol = [[NETunnelProviderProtocol alloc] init];
    tunnelProtocol.providerBundleIdentifier = [NSString stringWithUTF8String:VPN_NE_BUNDLEID];
    QByteArray configUtf8 = config.toUtf8();
    NSData *xrayConfigData = [NSData dataWithBytes:configUtf8.constData() length:configUtf8.size()];
    tunnelProtocol.providerConfiguration = @{@"xray": xrayConfigData};
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

    NETunnelProviderManager *tunnel = m_currentTunnel;
    [tunnel setEnabled:YES];

    dispatch_async(dispatch_get_main_queue(), ^{
        [tunnel saveToPreferencesWithCompletionHandler:^(NSError *saveError) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (saveError) {
                    qDebug().nospace() << "IosController::startTunnel" << protocolName << ": Connect " << protocolName
                                       << " Tunnel Save Error" << saveError.localizedDescription.UTF8String << " domain:"
                                       << saveError.domain.UTF8String << " code:" << saveError.code;
                    emit connectionStateChanged(Vpn::ConnectionState::Error);
                    return;
                }

                [tunnel loadFromPreferencesWithCompletionHandler:^(NSError *loadError) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (loadError) {
                            qDebug().nospace() << "IosController::startTunnel :" << tunnel.localizedDescription << protocolName
                                               << ": Connect " << protocolName << " Tunnel Load Error"
                                               << loadError.localizedDescription.UTF8String;
                            emit connectionStateChanged(Vpn::ConnectionState::Error);
                            return;
                        }

                        NSError *startError = nil;
                        qDebug() << iosStatusToState(tunnel.connection.status);

                        BOOL started = [tunnel.connection startVPNTunnelWithOptions:nil andReturnError:&startError];

                        if (!started || startError) {
                            qDebug().nospace() << "IosController::startTunnel :" << tunnel.localizedDescription << protocolName
                                               << " : Connect " << protocolName << " Tunnel Start Error"
                                               << (startError ? startError.localizedDescription.UTF8String : "");
                            emit connectionStateChanged(Vpn::ConnectionState::Error);
                        } else {
                            qDebug().nospace() << "IosController::startTunnel :" << tunnel.localizedDescription << protocolName
                                               << " : Starting the tunnel succeeded";
                        }
                    });
                }];
            });
        }];
    });
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
        if (callback) {
            callback(nil);
        }
        return;
    }

    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:message options:0 error:&error];

    if (!data || error) {
        qDebug() << "Failed to serialize message to VpnExtension as JSON. Error:"
                 << [error.localizedDescription UTF8String];
        if (callback) {
            callback(nil);
        }
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
        if (callback) {
            callback(nil);
        }
        return;
    }

    if (sendError) {
        qDebug() << "Failed to send message to VpnExtension. Error:"
                 << [sendError.localizedDescription UTF8String];
        if (callback) {
            callback(nil);
        }
    }

}

bool IosController::shareText(const QStringList& filesToSend) {
    NSMutableArray *sharingItems = [NSMutableArray new];

    for (int i = 0; i < filesToSend.size(); i++) {
        NSURL *logFileUrl = [[NSURL alloc] initFileURLWithPath:filesToSend[i].toNSString()];
        [sharingItems addObject:logFileUrl];
    }
#if !MACOS_NE
    UIViewController *qtController = getViewController();
    if (!qtController) {
        return false;
    }

    UIActivityViewController *activityController = [[UIActivityViewController alloc] initWithActivityItems:sharingItems applicationActivities:nil];
#endif
    __block bool isAccepted = false;
#if !MACOS_NE
    [activityController setCompletionWithItemsHandler:^(NSString *activityType, BOOL completed, NSArray *returnedItems, NSError *activityError) {
        isAccepted = completed;
        emit finished();
    }];

    [qtController presentViewController:activityController animated:YES completion:nil];
    UIPopoverPresentationController *popController = activityController.popoverPresentationController;
    if (popController) {
        popController.sourceView = qtController.view;
        popController.sourceRect = CGRectMake(100, 100, 100, 100);
    }

#endif
    QEventLoop wait;
    QObject::connect(this, &IosController::finished, &wait, &QEventLoop::quit);
    wait.exec();

    return isAccepted;
}

QString IosController::openFile() {
#if !MACOS_NE
    UIDocumentPickerViewController *documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.item"] inMode:UIDocumentPickerModeOpen];

    DocumentPickerDelegate *documentPickerDelegate = [[DocumentPickerDelegate alloc] init];
    documentPicker.delegate = documentPickerDelegate;

    UIViewController *qtController = getViewController();
    if (!qtController) return;

    [qtController presentViewController:documentPicker animated:YES completion:nil];

#endif
    __block QString filePath;
#if !MACOS_NE
    documentPickerDelegate.documentPickerClosedCallback = ^(NSString *path) {
        if (path) {
            filePath = QString::fromUtf8(path.UTF8String);
        } else {
            filePath = QString();
        }
        emit finished();
    };
#endif
    QEventLoop wait;
    QObject::connect(this, &IosController::finished, &wait, &QEventLoop::quit);
    wait.exec();

    return filePath;
}

namespace
{
// Keep in sync with StoreKit2Helper.errorCodeCancelled / errorCodePending
constexpr int storeKitErrorCodeCancelled = 1;
constexpr int storeKitErrorCodePending = 2;

IosController::StorePurchaseFailure storePurchaseFailureFromError(NSError *error)
{
    if (!error || ![error.domain isEqualToString:@"StoreKit2Helper"]) {
        return IosController::StorePurchaseFailure::Other;
    }
    switch (error.code) {
    case storeKitErrorCodeCancelled: return IosController::StorePurchaseFailure::Cancelled;
    case storeKitErrorCodePending: return IosController::StorePurchaseFailure::Pending;
    default: return IosController::StorePurchaseFailure::Other;
    }
}

QVariantMap toTransactionMap(NSDictionary *dict)
{
    QVariantMap transaction;
    for (NSString *key in @[@"transactionId", @"originalTransactionId", @"productId", @"environment"]) {
        NSString *value = dict[key];
        if (value) {
            transaction.insert(QString::fromUtf8(key.UTF8String), QString::fromUtf8(value.UTF8String));
        }
    }
    return transaction;
}

QList<QVariantMap> toTransactionList(NSArray<NSDictionary *> *transactions)
{
    QList<QVariantMap> list;
    for (NSDictionary *dict in transactions ?: @[]) {
        list.push_back(toTransactionMap(dict));
    }
    return list;
}
}

void IosController::purchaseProduct(const QString &productId,
                                   std::function<void(bool success,
                                                      const QString &transactionId,
                                                      const QString &purchasedProductId,
                                                      const QString &originalTransactionId,
                                                      const QString &storeEnvironment,
                                                      const QString &errorString,
                                                      StorePurchaseFailure failureReason)> &&callback)
{
    qInfo().noquote() << "[IAP][IosController] purchaseProduct called" << productId;
    if (@available(iOS 15.0, macOS 12.0, *)) {
        __block auto cb = std::move(callback);
        [[StoreKit2Helper shared] purchaseProductWithProductIdentifier:productId.toNSString()
                                                            completion:^(BOOL s,
                                                                         NSString * _Nullable transactionId,
                                                                         NSString * _Nullable prodId,
                                                                         NSString * _Nullable originalTxId,
                                                                         NSString * _Nullable environment,
                                                                         NSError * _Nullable error) {
            const QString txId = QString::fromUtf8((transactionId ?: @"").UTF8String);
            const QString pId  = QString::fromUtf8((prodId        ?: @"").UTF8String);
            const QString origTxId = QString::fromUtf8((originalTxId ?: @"").UTF8String);
            const QString env  = QString::fromUtf8((environment  ?: @"").UTF8String);
            const QString err  = QString::fromUtf8((error.localizedDescription ?: @"").UTF8String);
            const StorePurchaseFailure failureReason = s ? StorePurchaseFailure::Other
                                                         : storePurchaseFailureFromError(error);

            qInfo().noquote() << "[IAP][IosController] purchase completion" << "success=" << s
                              << "transactionId=" << txId << "originalTransactionId=" << origTxId
                              << "productId=" << pId << "environment=" << env << "error=" << err;

            if (cb) {
                cb(s, txId, pId, origTxId, env, err, failureReason);
            }
        }];
    } else {
        if (callback) {
            callback(false, QString(), QString(), QString(), QString(), "StoreKit 2 requires iOS 15.0 or later",
                     StorePurchaseFailure::Other);
        }
    }
}

void IosController::finishStoreTransaction(const QString &transactionId)
{
    if (transactionId.isEmpty()) {
        return;
    }
    if (@available(iOS 15.0, macOS 12.0, *)) {
        qInfo().noquote() << "[IAP][IosController] Finishing transaction" << transactionId;
        [[StoreKit2Helper shared] finishTransactionWithTransactionId:transactionId.toNSString()
                                                          completion:^(BOOL finished) {
            if (!finished) {
                qWarning().noquote() << "[IAP][IosController] Transaction was not found in the unfinished queue";
            }
        }];
    }
}

void IosController::startStoreTransactionObserver()
{
    if (@available(iOS 15.0, macOS 12.0, *)) {
        qInfo().noquote() << "[IAP][IosController] Starting transaction updates listener";
        [[StoreKit2Helper shared] startTransactionUpdatesListenerWithHandler:^(NSDictionary *transaction) {
            // Handler runs on the main GCD queue which shares the Qt main thread's run loop
            emit storeTransactionUpdated(toTransactionMap(transaction));
        }];
    }
}

void IosController::restorePurchases(std::function<void(bool success,
                                                       const QList<QVariantMap> &transactions,
                                                       const QString &errorString)> &&callback)
{
    if (@available(iOS 15.0, macOS 12.0, *)) {
        __block auto cb = std::move(callback);
        [[StoreKit2Helper shared] fetchCurrentEntitlementsWithCompletion:^(BOOL s,
                                                                           NSArray<NSDictionary *> * _Nullable restoredTransactions,
                                                                           NSError * _Nullable error) {
            QString err;
            if (error) {
                err = QString::fromUtf8(error.localizedDescription.UTF8String);
            }
            if (s) {
                qInfo().noquote() << "[IAP][IosController] currentEntitlements returned"
                                  << (int)(restoredTransactions ? restoredTransactions.count : 0) << "active entitlements";
            } else {
                qWarning().noquote() << "[IAP][IosController] fetchCurrentEntitlements failed:" << err;
            }
            if (cb) {
                cb(s, toTransactionList(restoredTransactions), err);
            }
        }];
    } else {
        if (callback) {
            callback(false, QList<QVariantMap>(), "StoreKit 2 requires iOS 15.0 or later");
        }
    }
}

void IosController::fetchLocalEntitlements(std::function<void(bool success,
                                                               const QList<QVariantMap> &transactions,
                                                               const QString &errorString)> &&callback)
{
    if (@available(iOS 15.0, macOS 12.0, *)) {
        __block auto cb = std::move(callback);
        [[StoreKit2Helper shared] fetchLocalEntitlementsWithCompletion:^(BOOL s,
                                                                         NSArray<NSDictionary *> * _Nullable entitlements,
                                                                         NSError * _Nullable error) {
            QString err;
            if (error) {
                err = QString::fromUtf8(error.localizedDescription.UTF8String);
            }
            if (cb) {
                cb(s, toTransactionList(entitlements), err);
            }
        }];
    } else {
        if (callback) {
            callback(false, QList<QVariantMap>(), "StoreKit 2 requires iOS 15.0 or later");
        }
    }
}

void IosController::fetchProducts(const QStringList &productIds,
                                  std::function<void(const QList<QVariantMap> &products,
                                                     const QStringList &invalidIds,
                                                     const QString &errorString)> &&callback)
{
    if (@available(iOS 15.0, macOS 12.0, *)) {
        NSMutableSet<NSString *> *ids = [NSMutableSet setWithCapacity:productIds.size()];
        for (const auto &pid : productIds) {
            [ids addObject:pid.toNSString()];
        }
        __block auto cb = std::move(callback);

        [[StoreKit2Helper shared] fetchProductsWithIdentifiers:ids
                                                    completion:^(NSArray<NSDictionary *> * _Nonnull products,
                                                                 NSArray<NSString *> * _Nonnull invalidIdentifiers,
                                                                 NSError * _Nullable error) {
            QList<QVariantMap> outProducts;
            for (NSDictionary *productInfo in products) {
                QVariantMap productData;
                productData["productId"] = QString::fromUtf8([productInfo[@"productId"] UTF8String]);
                productData["title"] = QString::fromUtf8([productInfo[@"title"] UTF8String]);
                productData["description"] = QString::fromUtf8([productInfo[@"description"] UTF8String]);
                productData["price"] = QString::fromUtf8([productInfo[@"price"] UTF8String]);
                if (productInfo[@"displayPrice"]) {
                    productData["displayPrice"] = QString::fromUtf8([productInfo[@"displayPrice"] UTF8String]);
                }
                productData["currencyCode"] = QString::fromUtf8([productInfo[@"currencyCode"] UTF8String]);
                if (productInfo[@"priceAmount"]) {
                    productData["priceAmount"] = [productInfo[@"priceAmount"] doubleValue];
                }
                if (productInfo[@"subscriptionBillingMonths"]) {
                    productData["subscriptionBillingMonths"] = [productInfo[@"subscriptionBillingMonths"] doubleValue];
                }
                if (productInfo[@"displayPricePerMonth"]) {
                    productData["displayPricePerMonth"] = QString::fromUtf8([productInfo[@"displayPricePerMonth"] UTF8String]);
                }
                if (productInfo[@"introOfferDisplayPrice"]) {
                    productData["introOfferDisplayPrice"] = QString::fromUtf8([productInfo[@"introOfferDisplayPrice"] UTF8String]);
                }
                if (productInfo[@"introOfferPaymentMode"]) {
                    productData["introOfferPaymentMode"] = QString::fromUtf8([productInfo[@"introOfferPaymentMode"] UTF8String]);
                }
                if (productInfo[@"hasFreeTrial"]) {
                    productData["hasFreeTrial"] = [productInfo[@"hasFreeTrial"] boolValue];
                }
                if (productInfo[@"trialDays"]) {
                    productData["trialDays"] = [productInfo[@"trialDays"] intValue];
                }
                outProducts.push_back(productData);
            }

            QStringList invalid;
            for (NSString *inv in invalidIdentifiers) {
                invalid.push_back(QString::fromUtf8(inv.UTF8String));
            }

            QString err;
            if (error) {
                err = QString::fromUtf8(error.localizedDescription.UTF8String);
            }

            if (cb) {
                cb(outProducts, invalid, err);
            }
        }];
    } else {
        if (callback) {
            callback(QList<QVariantMap>(), QStringList(), "StoreKit 2 requires iOS 15.0 or later");
        }
    }
}

void IosController::requestInetAccess() {
    NSURL *url = [NSURL URLWithString:@"http://captive.apple.com/generate_204"];
    if (!url) {
        qDebug() << "IosController::requestInetAccess URL error";
        return;
    }

    NSURLSession *session = [NSURLSession sharedSession];
    NSURLSessionDataTask *task = [session dataTaskWithURL:url completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if (error) {
            qDebug() << "IosController::requestInetAccess error:" << error.localizedDescription;
        } else {
            NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
            QString responseBody = QString::fromUtf8((const char*)data.bytes, data.length);
        }
    }];
    [task resume];
}

bool IosController::isTestFlight() {
    NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
    return receiptURL && [[receiptURL lastPathComponent] isEqualToString:@"sandboxReceipt"];
}

