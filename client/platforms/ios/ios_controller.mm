#include "ios_controller.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "../protocols/vpnprotocol.h"
#import "ios_controller_wrapper.h"

#import <NetworkExtension/NetworkExtension.h>
#import <NetworkExtension/NETunnelProviderManager.h>
#import <NetworkExtension/NEVPNManager.h>
#import <NetworkExtension/NETunnelProviderSession.h>


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

VpnProtocol::VpnConnectionState iosStatusToState(NEVPNStatus status) {
  switch (status) {
    case NEVPNStatusInvalid:
        return VpnProtocol::VpnConnectionState::Unknown;
    case NEVPNStatusDisconnected:
        return VpnProtocol::VpnConnectionState::Disconnected;
    case NEVPNStatusConnecting:
        return VpnProtocol::VpnConnectionState::Connecting;
    case NEVPNStatusConnected:
        return VpnProtocol::VpnConnectionState::Connected;
    case NEVPNStatusReasserting:
        return VpnProtocol::VpnConnectionState::Connecting;
    case NEVPNStatusDisconnecting:
        return VpnProtocol::VpnConnectionState::Disconnecting;
    default:
        return VpnProtocol::VpnConnectionState::Unknown;
}
}

namespace {
IosController* s_instance = nullptr;
}

IosController::IosController() : QObject()
{
    qDebug() << "IosController::IosController() init";
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
    qDebug() << "IosController::initialize";

    __block bool ok = true;
    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> * _Nullable managers, NSError * _Nullable error) {
        @try {
            if (error) {
                qDebug() << "IosController::initialize : Error:" << [error.localizedDescription UTF8String];
                emit connectionStateChanged(VpnProtocol::VpnConnectionState::Error);
                ok = false;
                return;
            }

            NSInteger managerCount = managers.count;
            qDebug() << "IosController::initialize : We have received managers:" << (long)managerCount;


            for (NETunnelProviderManager *manager in managers) {
                if (manager.connection.status == NEVPNStatusConnected) {
                    m_currentTunnel = manager;
                    qDebug() << "IosController::initialize : VPN already connected";
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
    qDebug() << "IosController::connectVpn called from thread:" << QThread::currentThread();

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

    // reset m_currentTunnel
    m_currentTunnel = nullptr;

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block bool ok = true;
    __block bool isNewTunnelCreated = false;

    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> * _Nullable managers, NSError * _Nullable error) {
        @try {
            if (error) {
                qDebug() << "IosController::connectVpn : Error:" << [error.localizedDescription UTF8String];
                emit connectionStateChanged(VpnProtocol::VpnConnectionState::Error);
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
                        emit connectionStateChanged(VpnProtocol::VpnConnectionState::Connected);
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
    getBackendLogs([](QString s){
        //qDebug().noquote() << s;
    });

    NETunnelProviderSession *session = (NETunnelProviderSession *)pNotification;

    if (session /* && session == TunnelManager.session */ ) {
       qDebug() << "IosController::vpnStatusDidChange" << iosStatusToState(session.status) << session;
       emit connectionStateChanged(iosStatusToState(session.status));
    }
}

void IosController::vpnConfigurationDidChange(void *pNotification)
{
    qDebug() << "IosController::vpnConfigurationDidChange" << pNotification;
}

bool IosController::setupOpenVPN()
{
    qDebug() << "IosController::setupOpenVPN";

    QJsonObject ovpn = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::OpenVpn)].toObject();
    QString ovpnConfig = ovpn[config_key::config].toString();

    return startOpenVPN(ovpnConfig);
}

bool IosController::setupCloak()
{
    qDebug() << "IosController::setupCloak";
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

    return startOpenVPN(ovpnConfig);
}

bool IosController::setupWireGuard()
{
    qDebug() << "IosController::setupWireGuard";
    QJsonObject config = m_rawConfig[ProtocolProps::key_proto_config_data(amnezia::Proto::WireGuard)].toObject();

    QString wgConfig = config[config_key::config].toString();
    
    return startWireGuard(wgConfig);
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
    m_rxBytes = 0;
    m_txBytes = 0;
    [m_currentTunnel setEnabled:YES];

    [m_currentTunnel saveToPreferencesWithCompletionHandler:^(NSError *saveError) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            qDebug() << "IosController::saveToPreferencesWithCompletionHandler called from thread:" << QThread::currentThread();

            if (saveError) {
                qDebug() << "IosController::startOpenVPN : Connect OpenVPN Tunnel Save Error" << saveError.localizedDescription.UTF8String;
                emit connectionStateChanged(VpnProtocol::VpnConnectionState::Error);
                return;
            }

            [m_currentTunnel loadFromPreferencesWithCompletionHandler:^(NSError *loadError) {
                    qDebug() << "IosController::loadFromPreferencesWithCompletionHandler called from thread:" << QThread::currentThread();

                    if (loadError) {
                        qDebug() << "IosController::startOpenVPN : Connect OpenVPN Tunnel Load Error" << loadError.localizedDescription.UTF8String;
                        emit connectionStateChanged(VpnProtocol::VpnConnectionState::Error);
                        return;
                    }

                    NSError *startError = nil;
                    qDebug() << iosStatusToState(m_currentTunnel.connection.status);


                    NSString *actionKey = [NSString stringWithUTF8String:MessageKey::action];
                    NSString *actionValue = [NSString stringWithUTF8String:Action::start];
                    NSString *tunnelIdKey = [NSString stringWithUTF8String:MessageKey::tunnelId];
                    NSString *tunnelIdValue = !m_tunnelId.isEmpty() ? m_tunnelId.toNSString() : @"";

                    NSDictionary* message = @{actionKey: actionValue, tunnelIdKey: tunnelIdValue};
                    sendVpnExtensionMessage(message, [&](NSDictionary* response){
                        qDebug() << "sendVpnExtensionMessage" << response;
                    });


                    BOOL started = [m_currentTunnel.connection startVPNTunnelWithOptions:nil andReturnError:&startError];

                    if (!started || startError) {
                        qDebug() << "IosController::startOpenVPN : Connect OpenVPN Tunnel Start Error"
                            << (startError ? startError.localizedDescription.UTF8String : "");
                        emit connectionStateChanged(VpnProtocol::VpnConnectionState::Error);
                    } else {
                        qDebug() << "IosController::startOpenVPN : Starting the tunnel succeeded";
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

void IosController::getBackendLogs(std::function<void (const QString &)> &&callback)
{
    qDebug() << "IosController::getBackendLogs";

    NSString *suiteName = @"group.org.amnezia.AmneziaVPN";
    NSString *logKey = @"logMessages";

    NSUserDefaults *sharedDefaults = [[NSUserDefaults alloc] initWithSuiteName:suiteName];
    NSArray *logs = [sharedDefaults arrayForKey:logKey];

    for (NSString *logMessage in logs) {
        qDebug() << QString::fromNSString(logMessage);
    }

    [sharedDefaults removeObjectForKey:logKey];
    [sharedDefaults synchronize];


//    std::function<void(const QString&)> a_callback = std::move(callback);

//    QString groupId(GROUP_ID);
//    NSURL* groupPath = [[NSFileManager defaultManager]
//        containerURLForSecurityApplicationGroupIdentifier:groupId.toNSString()];

//    NSURL* path = [groupPath URLByAppendingPathComponent:@"networkextension.log"];

//    QFile file(QString::fromNSString([path path]));
//    if (!file.open(QIODevice::ReadOnly)) {
//      a_callback("IosController::getBackendLogs : Network extension log file missing or unreadable.");
//      return;
//    }

//    QByteArray content = file.readAll();
//    qDebug() << "IosController::getBackendLogs" << content;
//    a_callback(content);
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
            callback(nil);
            return;
        }

        NSError *deserializeError = nil;
        NSDictionary *response = [NSJSONSerialization JSONObjectWithData:responseData options:0 error:&deserializeError];

        if (response && [response isKindOfClass:[NSDictionary class]]) {
           // qDebug() << "Received extension message:" << QString::fromNSString(response.description);;
            callback(response);
            return;
        } else if (deserializeError) {
            qDebug() << "Failed to deserialize the VpnExtension response";
        }

        callback(nil);
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

void IosController::onStartVpnExtensionMessage(NSDictionary* message, void(^callback)(NSDictionary*))
{
    qDebug() << "IosController::onStartVpnExtensionMessage";
//    if (!message) {
//        NSDictionary* errorDict = @{ @"error" : @(ErrorCode::vpnStartFailure) };
//        callback(errorDict);
//        return;
//    }

//    NSNumber* rawErrorCodeObj = message[MessageKey::errorCode];
//    int rawErrorCode = rawErrorCodeObj ? [rawErrorCodeObj intValue] : ErrorCode::undefined;

//    if (rawErrorCode == ErrorCode::noError) {
//        NSString* tunnelId = message[MessageKey::tunnelId];
//        if (tunnelId) {
//            self.activeTunnelId = std::string([tunnelId UTF8String]);
//            this->setConnectVpnOnDemand(true);
//        }
//    }

//    NSDictionary* responseDict = @{ @"errorCode" : @(rawErrorCode) };
//    callback(responseDict);
}

