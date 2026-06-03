#include "ikev2VpnProtocolMacos.h"

#include <QDebug>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

namespace {

NSData *copyIdentityPersistentRef(const QString &label)
{
    NSDictionary *lookupQuery = @{
        (__bridge id)kSecClass : (__bridge id)kSecClassIdentity,
        (__bridge id)kSecAttrLabel : label.toNSString(),
        (__bridge id)kSecMatchLimit : (__bridge id)kSecMatchLimitOne,
        (__bridge id)kSecReturnPersistentRef : @YES,
    };

    CFTypeRef persistentRef = NULL;
    OSStatus lookupStatus = SecItemCopyMatching((__bridge CFDictionaryRef)lookupQuery, &persistentRef);
    if (lookupStatus != errSecSuccess || persistentRef == NULL) {
        qCritical() << "[IKEv2-mac] keychain identity not found, status" << (int)lookupStatus;
        return nil;
    }

    return (NSData *)CFAutorelease(persistentRef);
}

bool importIdentityIntoKeychain(const QByteArray &certData, const QString &certPassword, const QString &label)
{
    NSData *p12Blob = [NSData dataWithBytes:certData.constData() length:certData.size()];
    NSDictionary *importOptions = @{ (__bridge id)kSecImportExportPassphrase : certPassword.toNSString() };

    CFArrayRef importedItems = NULL;
    OSStatus importStatus = SecPKCS12Import((__bridge CFDataRef)p12Blob, (__bridge CFDictionaryRef)importOptions, &importedItems);
    if (importStatus != errSecSuccess || importedItems == NULL) {
        qCritical() << "[IKEv2-mac] SecPKCS12Import failed, status" << (int)importStatus;
        if (importedItems) {
            CFRelease(importedItems);
        }
        return false;
    }

    NSDictionary *firstImport =
            CFArrayGetCount(importedItems) > 0 ? (NSDictionary *)CFArrayGetValueAtIndex(importedItems, 0) : nil;
    SecIdentityRef clientIdentity = (SecIdentityRef)[firstImport objectForKey:(__bridge id)kSecImportItemIdentity];
    if (clientIdentity == NULL) {
        qCritical() << "[IKEv2-mac] certificate contains no client identity";
        CFRelease(importedItems);
        return false;
    }

    NSDictionary *keychainEntry = @{
        (__bridge id)kSecValueRef : (__bridge id)clientIdentity,
        (__bridge id)kSecAttrLabel : label.toNSString(),
    };

    OSStatus storeStatus = SecItemAdd((__bridge CFDictionaryRef)keychainEntry, NULL);
    CFRelease(importedItems);

    if (storeStatus != errSecSuccess && storeStatus != errSecDuplicateItem) {
        qCritical() << "[IKEv2-mac] saving client identity to keychain failed, status" << (int)storeStatus;
        return false;
    }

    return true;
}

void removeIdentityFromKeychain(const QString &label)
{
    NSDictionary *deleteQuery = @{
        (__bridge id)kSecClass : (__bridge id)kSecClassIdentity,
        (__bridge id)kSecAttrLabel : label.toNSString(),
        (__bridge id)kSecMatchLimit : (__bridge id)kSecMatchLimitAll,
    };

    SecItemDelete((__bridge CFDictionaryRef)deleteQuery);
}

}  // namespace

Ikev2ProtocolMacos::Ikev2ProtocolMacos(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
    readIkev2Configuration(configuration);
}

Ikev2ProtocolMacos::~Ikev2ProtocolMacos()
{
    Ikev2ProtocolMacos::stop();
    removeStatusObserver();
}

void Ikev2ProtocolMacos::readIkev2Configuration(const QJsonObject &configuration)
{
    m_config = configuration.value(ProtocolUtils::key_proto_config_data(Proto::Ikev2)).toObject();

    m_hostName = m_config.value(configKey::hostName).toString();
    m_clientId = m_config.value(configKey::userName).toString();
    m_clientCertBase64 = m_config.value(configKey::cert).toString();
    m_clientCertPassword = m_config.value(configKey::password).toString();
}

bool Ikev2ProtocolMacos::storeClientIdentity()
{
    QByteArray certData = QByteArray::fromBase64(m_clientCertBase64.toUtf8());
    if (certData.isEmpty()) {
        qCritical() << "[IKEv2-mac] client certificate is not valid base64";
        return false;
    }

    removeIdentityFromKeychain(tunnelName());

    return importIdentityIntoKeychain(certData, m_clientCertPassword, tunnelName());
}

void Ikev2ProtocolMacos::removeStatusObserver()
{
    if (m_statusObserver) {
        NEVPNManager *manager = [NEVPNManager sharedManager];
        [[NSNotificationCenter defaultCenter] removeObserver:(id)m_statusObserver
                                                        name:NEVPNStatusDidChangeNotification
                                                      object:manager.connection];
        m_statusObserver = nullptr;
    }
}

ErrorCode Ikev2ProtocolMacos::start()
{
    if (m_hostName.isEmpty() || m_clientCertBase64.isEmpty()) {
        qCritical() << "[IKEv2-mac] missing server address or client certificate";
        setLastError(ErrorCode::IKEv2ConfigError);
        return ErrorCode::IKEv2ConfigError;
    }

    if (!storeClientIdentity()) {
        setLastError(ErrorCode::IKEv2ConfigError);
        return ErrorCode::IKEv2ConfigError;
    }

    m_handshakeTimedOut = false;
    m_lastVpnStatus = NEVPNStatusInvalid;

    setConnectionState(Vpn::ConnectionState::Connecting);

    NEVPNManager *manager = [NEVPNManager sharedManager];

    NSString *nsServerAddress = m_hostName.toNSString();
    NSString *nsLocalIdentifier = m_clientId.toNSString();

    [manager loadFromPreferencesWithCompletionHandler:^(NSError *loadError) {
      if (loadError) {
          qCritical() << "[IKEv2-mac] loading VPN preferences failed:"
                      << QString::fromNSString(loadError.localizedDescription);
          setLastError(ErrorCode::IKEv2LoadError);
          return;
      }

      NEVPNProtocolIKEv2 *protocol = [[NEVPNProtocolIKEv2 alloc] init];

      protocol.serverAddress = nsServerAddress;
      protocol.remoteIdentifier = nsServerAddress;
      protocol.localIdentifier = nsLocalIdentifier;

      protocol.authenticationMethod = NEVPNIKEAuthenticationMethodCertificate;
      protocol.certificateType = NEVPNIKEv2CertificateTypeRSA;
      protocol.identityReference = copyIdentityPersistentRef(Ikev2ProtocolMacos::tunnelName());
      protocol.useExtendedAuthentication = NO;
      protocol.enablePFS = NO;
      protocol.disconnectOnSleep = NO;
      protocol.deadPeerDetectionRate = NEVPNIKEv2DeadPeerDetectionRateMedium;

      protocol.IKESecurityAssociationParameters.encryptionAlgorithm = NEVPNIKEv2EncryptionAlgorithmAES256;
      protocol.IKESecurityAssociationParameters.integrityAlgorithm = NEVPNIKEv2IntegrityAlgorithmSHA256;
      protocol.IKESecurityAssociationParameters.diffieHellmanGroup = NEVPNIKEv2DiffieHellmanGroup14;
      protocol.IKESecurityAssociationParameters.lifetimeMinutes = 1410;

      protocol.childSecurityAssociationParameters.encryptionAlgorithm = NEVPNIKEv2EncryptionAlgorithmAES128GCM;
      protocol.childSecurityAssociationParameters.diffieHellmanGroup = NEVPNIKEv2DiffieHellmanGroup14;
      protocol.childSecurityAssociationParameters.lifetimeMinutes = 1410;

      [manager setProtocolConfiguration:protocol];
      [manager setLocalizedDescription:Ikev2ProtocolMacos::tunnelName().toNSString()];
      [manager setEnabled:YES];
      [manager setOnDemandEnabled:NO];

      [manager saveToPreferencesWithCompletionHandler:^(NSError *firstSaveError) {
        if (firstSaveError) {
            qCritical() << "[IKEv2-mac] saving VPN preferences failed:"
                        << QString::fromNSString(firstSaveError.localizedDescription);
            setLastError(ErrorCode::IKEv2SaveError);
            return;
        }

        [manager loadFromPreferencesWithCompletionHandler:^(NSError *reloadError) {
          if (reloadError) {
              qCritical() << "[IKEv2-mac] reloading VPN preferences failed:"
                          << QString::fromNSString(reloadError.localizedDescription);
              setLastError(ErrorCode::IKEv2LoadError);
              return;
          }

          [manager saveToPreferencesWithCompletionHandler:^(NSError *resaveError) {
            if (resaveError) {
                qCritical() << "[IKEv2-mac] re-saving VPN preferences failed:"
                            << QString::fromNSString(resaveError.localizedDescription);
                setLastError(ErrorCode::IKEv2SaveError);
                return;
            }

            removeStatusObserver();
            m_statusObserver = (void *)[[NSNotificationCenter defaultCenter]
                    addObserverForName:NEVPNStatusDidChangeNotification
                                object:manager.connection
                                 queue:nil
                            usingBlock:^(NSNotification *notification) {
                              NEVPNConnection *connection = notification.object;
                              this->handleStatusChange((int)connection.status);
                            }];

            NSError *tunnelStartError = nil;
            [manager.connection startVPNTunnelAndReturnError:&tunnelStartError];
            if (tunnelStartError) {
                qCritical() << "[IKEv2-mac] starting the tunnel failed:"
                            << QString::fromNSString(tunnelStartError.localizedDescription);
                setLastError(ErrorCode::IKEv2ConnectError);
            } else {
                startHandshakeTimeoutTimer();
            }
          }];
        }];
      }];
    }];

    return ErrorCode::NoError;
}

void Ikev2ProtocolMacos::stop()
{
    stopHandshakeTimeoutTimer();

    NEVPNManager *manager = [NEVPNManager sharedManager];
    NEVPNStatus status = manager.connection.status;

    if (status == NEVPNStatusDisconnected || status == NEVPNStatusInvalid) {
        removeStatusObserver();
        setConnectionState(Vpn::ConnectionState::Disconnected);
    } else {
        setConnectionState(Vpn::ConnectionState::Disconnecting);
        [manager.connection stopVPNTunnel];
    }
}

void Ikev2ProtocolMacos::handleStatusChange(int rawStatus)
{
    NEVPNStatus vpnStatus = static_cast<NEVPNStatus>(rawStatus);
    const Vpn::ConnectionState currentState = connectionState();

    switch (vpnStatus) {
    case NEVPNStatusConnecting:
        setConnectionState(Vpn::ConnectionState::Connecting);
        break;

    case NEVPNStatusConnected:
        stopHandshakeTimeoutTimer();
        m_lastVpnStatus = vpnStatus;
        qInfo() << "[IKEv2-mac] tunnel established";
        setConnectionState(Vpn::ConnectionState::Connected);
        break;

    case NEVPNStatusReasserting:
        m_lastVpnStatus = vpnStatus;
        setConnectionState(Vpn::ConnectionState::Reconnecting);
        break;

    case NEVPNStatusDisconnecting:
        stopHandshakeTimeoutTimer();
        setConnectionState(Vpn::ConnectionState::Disconnecting);
        break;

    case NEVPNStatusDisconnected: {
        stopHandshakeTimeoutTimer();
        removeStatusObserver();

        if (m_handshakeTimedOut) {
            qCritical() << "[IKEv2-mac] handshake timed out";
            setLastError(ErrorCode::IKEv2TimeoutError);
        } else if (m_lastVpnStatus == NEVPNStatusInvalid && currentState == Vpn::ConnectionState::Connecting) {
            qCritical() << "[IKEv2-mac] server rejected the configuration";
            setLastError(ErrorCode::IKEv2ConfigError);
        } else if (m_lastVpnStatus == NEVPNStatusConnected
                   && (currentState == Vpn::ConnectionState::Connecting
                       || currentState == Vpn::ConnectionState::Connected)) {
            qWarning() << "[IKEv2-mac] tunnel disabled from system settings";
            setConnectionState(Vpn::ConnectionState::Disconnected);
        } else {
            setConnectionState(Vpn::ConnectionState::Disconnected);
        }
        m_lastVpnStatus = vpnStatus;
        break;
    }

    case NEVPNStatusInvalid:
        stopHandshakeTimeoutTimer();
        removeStatusObserver();
        qCritical() << "[IKEv2-mac] VPN profile is invalid";
        m_lastVpnStatus = vpnStatus;
        setLastError(ErrorCode::IKEv2ConfigError);
        break;

    default:
        break;
    }
}

void Ikev2ProtocolMacos::startHandshakeTimeoutTimer()
{
    stopHandshakeTimeoutTimer();

    m_handshakeTimeoutTimer = new QTimer(this);
    m_handshakeTimeoutTimer->setSingleShot(true);
    m_handshakeTimeoutTimer->setInterval(HANDSHAKE_TIMEOUT_SEC * 1000);
    connect(m_handshakeTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (connectionState() == Vpn::ConnectionState::Connecting) {
            m_handshakeTimedOut = true;
            NEVPNManager *manager = [NEVPNManager sharedManager];
            [manager.connection stopVPNTunnel];
        }
    });
    m_handshakeTimeoutTimer->start();
}

void Ikev2ProtocolMacos::stopHandshakeTimeoutTimer()
{
    if (m_handshakeTimeoutTimer) {
        m_handshakeTimeoutTimer->stop();
        m_handshakeTimeoutTimer->deleteLater();
        m_handshakeTimeoutTimer = nullptr;
    }
}
