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
    qDebug() << "Initializing Swift Controller";
    
    
    qDebug() << "RECEIVED CONFIG FROM SERVER SIDE  =>";
    qDebug() << QJsonDocument(m_rawConfig).toJson();
    
    if (!m_controller) {
        bool ok;
        QtJson::JsonObject result = QtJson::parse(QJsonDocument(m_rawConfig).toJson(), ok).toMap();
        
        if(!ok) {
            qDebug() << QString("An error occurred during parsing");
            return false;
        }
        
        QString protoName = result["protocol"].toString();
        qDebug() << "PROTOCOL: " << protoName;

        if (protoName == "wireguard") {
            setupWireguardProtocol(result);
            currentProto = amnezia::Proto::WireGuard;
        } else if (protoName == "openvpn") {
            setupOpenVPNProtocol(result);
            currentProto = amnezia::Proto::OpenVpn;
        } else if (protoName == "shadowsocks") {
            setupShadowSocksProtocol(result);
            currentProto = amnezia::Proto::ShadowSocks;
        } else {
            return false;
        }
    }
    return true;
}


ErrorCode IOSVpnProtocol::start()
{
    bool ok;
    QtJson::JsonObject result = QtJson::parse(QJsonDocument(m_rawConfig).toJson(), ok).toMap();
    qDebug() << "current protocol: " << currentProto;
    qDebug() << "new protocol: " << m_protocol;
    qDebug() << "config: " << result;
    
    if(!ok) {
        qDebug() << QString("An error occurred during config parsing");
        return InternalError;
    }
    
    if (m_isChangingState)
        return NoError;
    
    QString protocol = result["protocol"].toString();
    
    if (!m_controller)
        initialize();
    
    switch (m_protocol) {
        case amnezia::Proto::OpenVpn:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchOpenVPNTunnel(result);
                currentProto = amnezia::Proto::OpenVpn;
                return NoError;
            }
            initialize();
            launchOpenVPNTunnel(result);
            break;
        case amnezia::Proto::WireGuard:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchWireguardTunnel(result);
                currentProto = amnezia::Proto::WireGuard;
                return NoError;
            }
            initialize();
            launchWireguardTunnel(result);
            break;
        case amnezia::Proto::ShadowSocks:
            if (currentProto != m_protocol) {
                if (m_controller) {
                    stop();
                    initialize();
                }
                launchShadowSocksTunnel(result);
                currentProto = amnezia::Proto::ShadowSocks;
                return NoError;
            }
            initialize();
            launchShadowSocksTunnel(result);
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
        
//        dispatch_async(dispatch_get_main_queue(), ^{
//            emit connectionStateChanged(Disconnected);
//        });

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
    qDebug() << "Checking status";
    
    if (m_checkingStatus) {
        qDebug() << "We are still waiting for the previous status.";
        return;
    }
    
    if (!m_controller) {
        qDebug() << "Not correctly initialized";
        return;
    }
    
    m_checkingStatus = true;
    
    [m_controller checkStatusWithCallback:^(NSString* serverIpv4Gateway, NSString* deviceIpv4Address,
                                            NSString* configString) {
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
        
        qDebug() << "ServerIpv4Gateway:" << QString::fromNSString(serverIpv4Gateway)
                    << "DeviceIpv4Address:" << QString::fromNSString(deviceIpv4Address)
                    << "RxBytes:" << rxBytes << "TxBytes:" << txBytes;
        emit newTransmittedDataCount(rxBytes, txBytes);
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

void IOSVpnProtocol::setupWireguardProtocol(const QtJson::JsonObject &result)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    QtJson::JsonObject config = result["wireguard_config_data"].toMap();
    
    QString privateKey = config["client_priv_key"].toString();
    QByteArray key = QByteArray::fromBase64(privateKey.toLocal8Bit());
    
    qDebug() << "  - " << "client_priv_key: " << config["client_priv_key"].toString();
    qDebug() << "  - " << "client_pub_key: " << config["client_pub_key"].toString();
    qDebug() << "  - " << "interface config: " << config["config"].toString();
    
    QString addr = config["config"].toString().split("\n").takeAt(1).split(" = ").takeLast();
    QString dns = config["config"].toString().split("\n").takeAt(2).split(" = ").takeLast();
    QString privkey = config["config"].toString().split("\n").takeAt(3).split(" = ").takeLast();
    QString pubkey = config["config"].toString().split("\n").takeAt(6).split(" = ").takeLast();
    QString presharedkey = config["config"].toString().split("\n").takeAt(7).split(" = ").takeLast();
    QString allowedips = config["config"].toString().split("\n").takeAt(8).split(" = ").takeLast();
    QString endpoint = config["config"].toString().split("\n").takeAt(9).split(" = ").takeLast();
    QString keepalive = config["config"].toString().split("\n").takeAt(10).split(" = ").takeLast();
    qDebug() << "  - " << "[Interface] address: " << addr;
    qDebug() << "  - " << "[Interface] dns: " << dns;
    qDebug() << "  - " << "[Interface] private key: " << privkey;
    qDebug() << "  - " << "[Peer] public key: " << pubkey;
    qDebug() << "  - " << "[Peer] preshared key: " << presharedkey;
    qDebug() << "  - " << "[Peer] allowed ips: " << allowedips;
    qDebug() << "  - " << "[Peer] endpoint: " << endpoint;
    qDebug() << "  - " << "[Peer] keepalive: " << keepalive;
    
    qDebug() << "  - " << "hostName: " << config["hostName"].toString();
    qDebug() << "  - " << "psk_key: " << config["psk_key"].toString();
    qDebug() << "  - " << "server_pub_key: " << config["server_pub_key"].toString();
    
    m_controller = [[IOSVpnProtocolImpl alloc] initWithBundleID:@VPN_NE_BUNDLEID
                                                     privateKey:key.toNSData()
                                              deviceIpv4Address:addr.toNSString()
                                              deviceIpv6Address:@"::/0"
    closure:^(ConnectionState state, NSDate* date) {
        qDebug() << "Creation completed with connection state:" << state;
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
//                QDateTime qtDate(QDateTime::fromNSDate(date));
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                // Just in case we are connecting, let's call disconnect.
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

void IOSVpnProtocol::setupOpenVPNProtocol(const QtJson::JsonObject &result)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    QtJson::JsonObject ovpn = result["openvpn_config_data"].toMap();
    QString ovpnConfig = ovpn["config"].toString();
//    qDebug() << ovpn;
    
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
//                QDateTime qtDate(QDateTime::fromNSDate(date));
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                // Just in case we are connecting, let's call disconnect.
//                [m_controller disconnect];
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
        qDebug() << "OVPN State changed: " << a_connected;
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

void IOSVpnProtocol::setupShadowSocksProtocol(const QtJson::JsonObject &result)
{
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    QtJson::JsonObject ovpn = result["openvpn_config_data"].toMap();
    QString ovpnConfig = ovpn["config"].toString();
    qDebug() << "OpenVPN Config:\n" << ovpn;
    QtJson::JsonObject ssConfig = result["shadowsocks_config_data"].toMap();
    
    m_controller = [[IOSVpnProtocolImpl alloc] initWithBundleID:@VPN_NE_BUNDLEID
                                                   tunnelConfig:ovpnConfig.toNSString()
                                                       ssConfig:serializeSSConfig(ssConfig).toNSString()
    closure:^(ConnectionState state, NSDate* date) {
        qDebug() << "ShadowSocks creation completed with connection state:" << state;
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
    //                QDateTime qtDate(QDateTime::fromNSDate(date));
                dispatch_async(dispatch_get_main_queue(), ^{
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    m_isChangingState = false;
                });
                return;
            }
            case ConnectionStateDisconnected:
                // Just in case we are connecting, let's call disconnect.
    //                [m_controller disconnect];
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

void IOSVpnProtocol::launchWireguardTunnel(const QtJson::JsonObject &result)
{
    QtJson::JsonObject config = result["wireguard_config_data"].toMap();
    
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
    
    qDebug() << "IOSVPNProtocol starts for" << hostname;
    qDebug() << "DNS:" << dnsServersList.takeFirst().toNSString();
    qDebug() << "serverPublicKey:" << serverPubKey.toNSString();
    qDebug() << "serverIpv4AddrIn:" << serverAddr.toNSString();
    qDebug() << "serverPort:" << (uint32_t)port.toInt();
    qDebug() << "allowed ip list" << allowedIPList;
    
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

void IOSVpnProtocol::launchOpenVPNTunnel(const QtJson::JsonObject &result)
{
    QtJson::JsonObject ovpn = result["openvpn_config_data"].toMap();
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

void IOSVpnProtocol::launchShadowSocksTunnel(const QtJson::JsonObject &result) {
    QtJson::JsonObject ovpn = result["openvpn_config_data"].toMap();
    QString ovpnConfig = ovpn["config"].toString();
    QtJson::JsonObject ssConfig = result["shadowsocks_config_data"].toMap();
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

QString IOSVpnProtocol::serializeSSConfig(const QtJson::JsonObject &ssConfig) {
    QString ssLocalPort = ssConfig["local_port"].toString();
    QString ssMethod = ssConfig["method"].toString();
    QString ssPassword = ssConfig["password"].toString();
    QString ssServer = ssConfig["server"].toString();
    QString ssPort = ssConfig["server_port"].toString();
    QString ssTimeout = ssConfig["timeout"].toString();
    qDebug() << "\n\nSS CONFIG:";
    qDebug() << " local port -" << ssLocalPort;
    qDebug() << " method     -" << ssMethod;
    qDebug() << " password   -" << ssPassword;
    qDebug() << " server     -" << ssServer;
    qDebug() << " port       -" << ssPort;
    qDebug() << " timeout    -" << ssTimeout;
    
    QJsonObject shadowSocksConfig = QJsonObject();
    shadowSocksConfig.insert("local_addr", "127.0.0.1");
    shadowSocksConfig.insert("local_port", ssConfig["local_port"].toInt());
    shadowSocksConfig.insert("method", ssConfig["method"].toString());
//    shadowSocksConfig.insert("method", "aes-256-gcm");
    shadowSocksConfig.insert("password", ssConfig["password"].toString());
    shadowSocksConfig.insert("server", ssConfig["server"].toString());
    shadowSocksConfig.insert("server_port", ssConfig["server_port"].toInt());
    shadowSocksConfig.insert("timeout", ssConfig["timeout"].toInt());
    
    QString ss = QString(QJsonDocument(shadowSocksConfig).toJson());
    qDebug() << "SS Config JsonString:\n" << ss;
    return ss;
}
