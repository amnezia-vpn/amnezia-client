#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTextCodec>
#include <QTimer>
#include <QFile>

#include <QByteArray>

#include "platforms/ios/ipaddressrange.h"
#include "ios_vpnprotocol.h"
#include "core/errorstrings.h"
#include "AmneziaVPN-Swift.h"
#include "UIKit/UIKit.h"


namespace
{
IOSVpnProtocol* s_instance = nullptr;
IOSVpnProtocolImpl* m_controller = nullptr;
Proto currentProto = amnezia::Proto::Any;
}

IOSVpnProtocol::IOSVpnProtocol(Proto proto, const QJsonObject &configuration, QObject* parent)
: VpnProtocol(configuration, parent), m_protocol(proto)
{
    connect(this, &IOSVpnProtocol::newTransmittedDataCount, this, &IOSVpnProtocol::setBytesChanged);
}

IOSVpnProtocol* IOSVpnProtocol::instance() {
    return s_instance;
}

bool IOSVpnProtocol::initialize()
{   
    if (!m_controller) {       
        QString protoName = m_rawConfig["protocol"].toString();

        if (protoName == "wireguard") {
            setupWireguardProtocol(m_rawConfig);
            currentProto = amnezia::Proto::WireGuard;
        } else if (protoName == "openvpn") {
            setupOpenVPNProtocol(m_rawConfig);
            currentProto = amnezia::Proto::OpenVpn;
        } else if (protoName == "shadowsocks") {
            setupShadowSocksProtocol(m_rawConfig);
            currentProto = amnezia::Proto::ShadowSocks;
        } else if (protoName == "cloak") {
            setupCloakProtocol(m_rawConfig);
            currentProto = amnezia::Proto::Cloak;
        } else {
            return false;
        }
    }
    return true;
}


ErrorCode IOSVpnProtocol::start()
{   
    if (m_isChangingState)
        return NoError;
        
    if (!m_controller)
        initialize();
    
    switch (m_protocol) {
        case amnezia::Proto::Cloak:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchCloakTunnel(m_rawConfig);
                currentProto = amnezia::Proto::OpenVpn;
                return NoError;
            }
            initialize();
            launchCloakTunnel(m_rawConfig);
            break;
        case amnezia::Proto::OpenVpn:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchOpenVPNTunnel(m_rawConfig);
                currentProto = amnezia::Proto::OpenVpn;
                return NoError;
            }
            initialize();
            launchOpenVPNTunnel(m_rawConfig);
            break;
        case amnezia::Proto::WireGuard:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchWireguardTunnel(m_rawConfig);
                currentProto = amnezia::Proto::WireGuard;
                return NoError;
            }
            initialize();
            launchWireguardTunnel(m_rawConfig);
            break;
        case amnezia::Proto::ShadowSocks:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchShadowSocksTunnel(m_rawConfig);
                currentProto = amnezia::Proto::ShadowSocks;
                return NoError;
            }
            initialize();
            launchShadowSocksTunnel(m_rawConfig);
            break;
        default:
            break;
    }
    
    return NoError;
}

void IOSVpnProtocol::stop()
{
    if (!m_controller) {
        qDebug() << "Not correctly initialized";
        return;
    }
    
    [m_controller disconnect];
    
    emit connectionStateChanged(Disconnected);
    
    [m_controller dealloc];
    m_controller = nullptr;
}

void IOSVpnProtocol::resume_start()
{
    
}

void IOSVpnProtocol::checkStatus()
{   
    if (m_checkingStatus) {
        return;
    }
    
    if (!m_controller) {
        return;
    }
    
    m_checkingStatus = true;

    QPointer<IOSVpnProtocol> weakSelf = this;
    
    [m_controller checkStatusWithCallback:^(NSString* serverIpv4Gateway, NSString* deviceIpv4Address,
                                            NSString* configString) {
        if (!weakSelf) return;
        QString config = QString::fromNSString(configString);
        
        m_checkingStatus = false;
        
        if (config.isEmpty()) {
            return;
        }
        
        uint64_t txBytes = 0;
        uint64_t rxBytes = 0;
        
        QStringList lines = config.split("\n");
        for (const QString& line : lines) {
            if (line.startsWith("tx_bytes=")) {
                txBytes = line.split("=")[1].toULongLong();
            } else if (line.startsWith("rx_bytes=")) {
                rxBytes = line.split("=")[1].toULongLong();
            }
            
            if (txBytes && rxBytes) {
                break;
            }
        }
        
        emit weakSelf->newTransmittedDataCount(rxBytes, txBytes);
    }];
}

void IOSVpnProtocol::setNotificationText(const QString &title, const QString &message, int timerSec)
{
    // TODO: add user notifications?
}

void IOSVpnProtocol::setFallbackConnectedNotification()
{
    // TODO: add default user notifications?
}

void IOSVpnProtocol::getBackendLogs(std::function<void (const QString &)> &&callback)
{
    std::function<void(const QString&)> a_callback = std::move(callback);

    QString groupId(GROUP_ID);
    NSURL* groupPath = [[NSFileManager defaultManager]
        containerURLForSecurityApplicationGroupIdentifier:groupId.toNSString()];

    NSURL* path = [groupPath URLByAppendingPathComponent:@"networkextension.log"];

    QFile file(QString::fromNSString([path path]));
    if (!file.open(QIODevice::ReadOnly)) {
      a_callback("Network extension log file missing or unreadable.");
      return;
    }

    QByteArray content = file.readAll();
    a_callback(content);
}

void IOSVpnProtocol::cleanupBackendLogs()
{
    QString groupId(GROUP_ID);
    NSURL* groupPath = [[NSFileManager defaultManager]
        containerURLForSecurityApplicationGroupIdentifier:groupId.toNSString()];

    NSURL* path = [groupPath URLByAppendingPathComponent:@"networkextension.log"];

    QFile file(QString::fromNSString([path path]));
    file.remove();
}

void IOSVpnProtocol::setupWireguardProtocol(const QJsonObject& rawConfig)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    QJsonObject config = rawConfig["wireguard_config_data"].toObject();
    
    QString privateKey = config["client_priv_key"].toString();
    QByteArray key = QByteArray::fromBase64(privateKey.toLocal8Bit());
    
    QString addr = config["config"].toString().split("\n").takeAt(1).split(" = ").takeLast();
    QString dns = config["config"].toString().split("\n").takeAt(2).split(" = ").takeLast();
    QString privkey = config["config"].toString().split("\n").takeAt(3).split(" = ").takeLast();
    QString pubkey = config["config"].toString().split("\n").takeAt(6).split(" = ").takeLast();
    QString presharedkey = config["config"].toString().split("\n").takeAt(7).split(" = ").takeLast();
    QString allowedips = config["config"].toString().split("\n").takeAt(8).split(" = ").takeLast();
    QString endpoint = config["config"].toString().split("\n").takeAt(9).split(" = ").takeLast();
    QString keepalive = config["config"].toString().split("\n").takeAt(10).split(" = ").takeLast();
    
    m_controller = [[IOSVpnProtocolImpl alloc] initWithBundleID:@VPN_NE_BUNDLEID
                                                     privateKey:key.toNSData()
                                              deviceIpv4Address:addr.toNSString()
                                              deviceIpv6Address:@"::/0"
    closure:^(ConnectionState state, NSDate* date) {
        creating = false;
        
        switch (state) {
            case ConnectionStateError: {
                [m_controller dealloc];
                m_controller = nullptr;
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Error);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateConnected: {
                Q_ASSERT(date);
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                [m_controller disconnect];
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Disconnected);
                    m_isChangingState = false;
                });
                return;
        }
    }
    callback:^(BOOL a_connected) {
        if (currentProto != m_protocol) {
            qDebug() << "Protocols switched: " << a_connected;
            return;
        }
        qDebug() << "State changed: " << a_connected;
        if (a_connected) {
            dispatch_async(dispatch_get_main_queue(), ^{
                emit connectionStateChanged(Connected);
                m_isChangingState = false;
            });
            return;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}

void IOSVpnProtocol::setupCloakProtocol(const QJsonObject &rawConfig)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    QJsonObject ovpn = rawConfig["openvpn_config_data"].toObject();
    QString ovpnConfig = ovpn["config"].toString();
    
    m_controller = [[IOSVpnProtocolImpl alloc] initWithBundleID:@VPN_NE_BUNDLEID
                                                         config:ovpnConfig.toNSString()
    closure:^(ConnectionState state, NSDate* date) {
        creating = false;
        
        switch (state) {
            case ConnectionStateError: {
                [m_controller dealloc];
                m_controller = nullptr;
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Error);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateConnected: {
                Q_ASSERT(date);
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                // Just in case we are connecting, let's call disconnect.
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Disconnected);
                    m_isChangingState = false;
                });
                return;
        }
    }
    callback:^(BOOL a_connected) {
        if (currentProto != m_protocol) {
            qDebug() << "Protocols switched: " << a_connected;
            return;
        }
        qDebug() << "VPN State changed: " << a_connected;
        if (a_connected) {
            dispatch_async(dispatch_get_main_queue(), ^{
                emit connectionStateChanged(Connected);
                m_isChangingState = false;
            });
            return;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}

void IOSVpnProtocol::setupOpenVPNProtocol(const QJsonObject &rawConfig)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    QJsonObject ovpn = rawConfig["openvpn_config_data"].toObject();
    QString ovpnConfig = ovpn["config"].toString();
    
    m_controller = [[IOSVpnProtocolImpl alloc] initWithBundleID:@VPN_NE_BUNDLEID
                                                         config:ovpnConfig.toNSString()
    closure:^(ConnectionState state, NSDate* date) {
        qDebug() << "OVPN Creation completed with connection state:" << state;
        creating = false;
        
        switch (state) {
            case ConnectionStateError: {
                [m_controller dealloc];
                m_controller = nullptr;
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Error);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateConnected: {
                Q_ASSERT(date);
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                // Just in case we are connecting, let's call disconnect.
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Disconnected);
                    m_isChangingState = false;
                });
                return;
        }
    }
    callback:^(BOOL a_connected) {
        if (currentProto != m_protocol) {
            qDebug() << "Protocols switched: " << a_connected;
            return;
        }
        qDebug() << "VPN State changed: " << a_connected;
        if (a_connected) {
            dispatch_async(dispatch_get_main_queue(), ^{
                emit connectionStateChanged(Connected);
                m_isChangingState = false;
            });
            return;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}

void IOSVpnProtocol::setupShadowSocksProtocol(const QJsonObject &rawConfig)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    QJsonObject ovpn = rawConfig["openvpn_config_data"].toObject();
    QString ovpnConfig = ovpn["config"].toString();
    QJsonObject ssConfig = rawConfig["shadowsocks_config_data"].toObject();
    
    m_controller = [[IOSVpnProtocolImpl alloc] initWithBundleID:@VPN_NE_BUNDLEID
                                                   tunnelConfig:ovpnConfig.toNSString()
                                                       ssConfig:serializeSSConfig(ssConfig).toNSString()
    closure:^(ConnectionState state, NSDate* date) {
        creating = false;
        
        switch (state) {
            case ConnectionStateError: {
                [m_controller dealloc];
                m_controller = nullptr;
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Error);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateConnected: {
                Q_ASSERT(date);
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                // Just in case we are connecting, let's call disconnect.
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Disconnected);
                    m_isChangingState = false;
                });
                return;
        }
    }
    callback:^(BOOL a_connected) {
        if (currentProto != m_protocol) {
            qDebug() << "Protocols switched: " << a_connected;
            return;
        }
        qDebug() << "SS State changed: " << a_connected;
        if (a_connected) {
            dispatch_async(dispatch_get_main_queue(), ^{
                emit connectionStateChanged(Connected);
                m_isChangingState = false;
            });
            return;
        }
            dispatch_async(dispatch_get_main_queue(), ^{
                emit connectionStateChanged(Disconnected);
                m_isChangingState = false;
            });
    }];
}

void IOSVpnProtocol::launchWireguardTunnel(const QJsonObject &rawConfig)
{
    QJsonObject config = rawConfig["wireguard_config_data"].toObject();
    
    QString clientPrivateKey = config["client_priv_key"].toString();
    QByteArray key = QByteArray::fromBase64(clientPrivateKey.toLocal8Bit());
    QString clientPubKey = config["client_pub_key"].toString();
    
    QString addr = config["config"].toString().split("\n").takeAt(1).split(" = ").takeLast();
    QStringList dnsServersList = config["config"].toString().split("\n").takeAt(2).split(" = ").takeLast().split(", ");
    QString privkey = config["config"].toString().split("\n").takeAt(3).split(" = ").takeLast();
    QString pubkey = config["config"].toString().split("\n").takeAt(6).split(" = ").takeLast();
    QString presharedkey = config["config"].toString().split("\n").takeAt(7).split(" = ").takeLast();
    QStringList allowedIPList = config["config"].toString().split("\n").takeAt(8).split(" = ").takeLast().split(", ");
    QString endpoint = config["config"].toString().split("\n").takeAt(9).split(" = ").takeLast();
    QString serverAddr = config["config"].toString().split("\n").takeAt(9).split(" = ").takeLast().split(":").takeFirst();
    QString port = config["config"].toString().split("\n").takeAt(9).split(" = ").takeLast().split(":").takeLast();
    QString keepalive = config["config"].toString().split("\n").takeAt(10).split(" = ").takeLast();
    
    QString hostname = config["hostName"].toString();
    QString pskKey = config["psk_key"].toString();
    QString serverPubKey = config["server_pub_key"].toString();
    
    NSMutableArray<VPNIPAddressRange*>* allowedIPAddressRangesNS =
        [NSMutableArray<VPNIPAddressRange*> arrayWithCapacity:allowedIPList.length()];
    for (const IPAddressRange item : allowedIPList) {
        VPNIPAddressRange* range =
            [[VPNIPAddressRange alloc] initWithAddress:item.ipAddress().toNSString()
                                   networkPrefixLength:item.range()
                                                isIpv6:item.type() == IPAddressRange::IPv6];
        [allowedIPAddressRangesNS addObject:[range autorelease]];
    }
    
    [m_controller connectWithDnsServer:dnsServersList.takeFirst().toNSString()
                     serverIpv6Gateway:@"FE80::1"
                       serverPublicKey:serverPubKey.toNSString()
                          presharedKey:pskKey.toNSString()
                      serverIpv4AddrIn:serverAddr.toNSString()
                            serverPort:port.toInt()
                allowedIPAddressRanges:allowedIPAddressRangesNS
                           ipv6Enabled:NO
                                reason:0
                       failureCallback:^() {
        qDebug() << "Wireguard Protocol - connection failed";
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}


void IOSVpnProtocol::launchCloakTunnel(const QJsonObject &rawConfig)
{
    //TODO move to OpenVpnConfigurator
    QJsonObject ovpn = rawConfig["openvpn_config_data"].toObject();

    QString ovpnConfig = ovpn["config"].toString();
    
    if(rawConfig["protocol"].toString() == "cloak"){
        QJsonObject cloak = rawConfig["cloak_config_data"].toObject();
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

        // Convert JSONObject to JSONDocument
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
    }
    
    
    [m_controller connectWithOvpnConfig:ovpnConfig.toNSString()
                        failureCallback:^{
        qDebug() << "IOSVPNProtocol (OpenVPN Cloak) - connection failed";
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}



void IOSVpnProtocol::launchOpenVPNTunnel(const QJsonObject &rawConfig)
{
    QJsonObject ovpn = rawConfig["openvpn_config_data"].toObject();
    QString ovpnConfig = ovpn["config"].toString();
    
    [m_controller connectWithOvpnConfig:ovpnConfig.toNSString()
                        failureCallback:^{
        qDebug() << "IOSVPNProtocol (OpenVPN) - connection failed";
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}

void IOSVpnProtocol::launchShadowSocksTunnel(const QJsonObject &rawConfig) {
    QJsonObject ovpn = rawConfig["openvpn_config_data"].toObject();
    QString ovpnConfig = ovpn["config"].toString();
    QJsonObject ssConfig = rawConfig["shadowsocks_config_data"].toObject();
    QString ss = serializeSSConfig(ssConfig);
    
    [m_controller connectWithSsConfig:ss.toNSString()
                           ovpnConfig:ovpnConfig.toNSString()
                      failureCallback:^{
        qDebug() << "IOSVPNProtocol (ShadowSocks) - connection failed";
        dispatch_async(dispatch_get_main_queue(), ^{
            emit connectionStateChanged(Disconnected);
            m_isChangingState = false;
        });
    }];
}

QString IOSVpnProtocol::serializeSSConfig(const QJsonObject &ssConfig) {
    QString ssLocalPort = ssConfig["local_port"].toString();
    QString ssMethod = ssConfig["method"].toString();
    QString ssPassword = ssConfig["password"].toString();
    QString ssServer = ssConfig["server"].toString();
    QString ssPort = ssConfig["server_port"].toString();
    QString ssTimeout = ssConfig["timeout"].toString();
    
    QJsonObject shadowSocksConfig = QJsonObject();
    shadowSocksConfig.insert("local_addr", "127.0.0.1");
    shadowSocksConfig.insert("local_port", ssConfig["local_port"].toInt());
    shadowSocksConfig.insert("method", ssConfig["method"].toString());
    shadowSocksConfig.insert("password", ssConfig["password"].toString());
    shadowSocksConfig.insert("server", ssConfig["server"].toString());
    shadowSocksConfig.insert("server_port", ssConfig["server_port"].toInt());
    shadowSocksConfig.insert("timeout", ssConfig["timeout"].toInt());
    
    QString ss = QString(QJsonDocument(shadowSocksConfig).toJson());
    qDebug() << "SS Config JsonString:\n" << ss;
    return ss;
}
