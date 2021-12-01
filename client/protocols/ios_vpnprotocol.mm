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

#include "json.h"

#include "ipaddressrange.h"
#include "ios_vpnprotocol.h"
#include "core/errorstrings.h"
#include "AmneziaVPN-Swift.h"

namespace
{
IOSVpnProtocol* s_instance = nullptr;
IOSVpnProtocolImpl* m_controller = nullptr;
}

IOSVpnProtocol::IOSVpnProtocol(Proto proto, const QJsonObject &configuration, QObject* parent)
: VpnProtocol(configuration, parent),
m_protocol(proto) {}

IOSVpnProtocol* IOSVpnProtocol::instance() {
    return s_instance;
}

bool IOSVpnProtocol::initialize()
{
    qDebug() << "Initializing Swift Controller";
    
    static bool creating = false;
    // No nested creation!
    Q_ASSERT(creating == false);
    creating = true;
    
    if (!m_controller) {
        bool ok;
        QtJson::JsonObject result = QtJson::parse(QJsonDocument(m_rawConfig).toJson(), ok).toMap();
        
        if(!ok) {
            qDebug() << QString("An error occurred during parsing");
            return false;
        }
        
        QString vpnProto = result["protocol"].toString();
        qDebug() << "protocol: " << vpnProto;
        qDebug() << "config data => ";
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
                    emit connectionStateChanged(VpnConnectionState::Error);
                    return;
                }
                case ConnectionStateConnected: {
                    Q_ASSERT(date);
                    QDateTime qtDate(QDateTime::fromNSDate(date));
                    emit connectionStateChanged(VpnConnectionState::Connected);
                    return;
                }
                case ConnectionStateDisconnected:
                    // Just in case we are connecting, let's call disconnect.
                    [m_controller disconnect];
                    emit connectionStateChanged(VpnConnectionState::Disconnected);
                    return;
            }
        }
        callback:^(BOOL a_connected) {
            qDebug() << "State changed: " << a_connected;
            if (a_connected) {
                emit connectionStateChanged(Connected);
                return;
            }
//            emit connectionStateChanged(Disconnected);
        }];
    }
    return true;
}

ErrorCode IOSVpnProtocol::start()
{
    bool ok;
    QtJson::JsonObject result = QtJson::parse(QJsonDocument(m_rawConfig).toJson(), ok).toMap();
    
    if(!ok) {
        qDebug() << QString("An error occurred during config parsing");
        return InternalError;
    }
    QString protocol = result["protocol"].toString();
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
        qDebug() << "IOSVPNProtocol - connection failed";
        emit connectionStateChanged(Disconnected);
    }];
    return NoError;
}

void IOSVpnProtocol::stop()
{
    if (!m_controller) {
        qDebug() << "Not correctly initialized";
        emit connectionStateChanged(Disconnected);
        return;
    }
    
    [m_controller disconnect];
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
        emit bytesChanged(rxBytes, txBytes);
        
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

