#include "ikev2VpnProtocolMacos.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QTemporaryFile>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/ipcClient.h"
#include "ipc.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/x509.h>

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <Security/Security.h>

namespace {

// macOS rejects password-less PKCS#12 containers on import, so the client p12
// (exported by the Amnezia server without a password) is re-wrapped with a known
// password using legacy 3DES/SHA1 algorithms that Apple's importer accepts.
const char *kRepackedP12Password = "amnezia";

const char *kVpnSystemKeychainPath = "/Library/Keychains/System.keychain";

const char *vpnStatusName(int status)
{
    switch (status) {
    case NEVPNStatusInvalid: return "Invalid";
    case NEVPNStatusDisconnected: return "Disconnected";
    case NEVPNStatusConnecting: return "Connecting";
    case NEVPNStatusConnected: return "Connected";
    case NEVPNStatusReasserting: return "Reasserting";
    case NEVPNStatusDisconnecting: return "Disconnecting";
    default: return "Unknown";
    }
}

// Parses the password-less client p12, re-wraps the identity with a password and
// also extracts the CA certificate (DER) bundled in the p12 chain, which is needed
// to validate the self-signed server certificate.
bool prepareIdentity(const QByteArray &source, const QString &friendlyName, QByteArray &repackedP12, QByteArray &caCertDer)
{
    const unsigned char *cursor = reinterpret_cast<const unsigned char *>(source.constData());
    PKCS12 *sourceP12 = d2i_PKCS12(nullptr, &cursor, source.size());
    if (!sourceP12) {
        qCritical() << "[IKEv2-mac] failed to parse client p12 container";
        return false;
    }

    EVP_PKEY *privateKey = nullptr;
    X509 *certificate = nullptr;
    STACK_OF(X509) *caChain = nullptr;
    int parsed = PKCS12_parse(sourceP12, "", &privateKey, &certificate, &caChain);
    PKCS12_free(sourceP12);

    if (!parsed || !privateKey || !certificate) {
        qCritical() << "[IKEv2-mac] failed to extract key/certificate from client p12";
        if (privateKey) EVP_PKEY_free(privateKey);
        if (certificate) X509_free(certificate);
        if (caChain) sk_X509_pop_free(caChain, X509_free);
        return false;
    }

    const int caCount = caChain ? sk_X509_num(caChain) : 0;
    qInfo() << "[IKEv2-mac] CA certificates bundled in client p12:" << caCount;
    if (caCount > 0) {
        X509 *caCert = sk_X509_value(caChain, caCount - 1);
        unsigned char *caEncoded = nullptr;
        int caLength = i2d_X509(caCert, &caEncoded);
        if (caLength > 0 && caEncoded) {
            caCertDer = QByteArray(reinterpret_cast<const char *>(caEncoded), caLength);
            OPENSSL_free(caEncoded);
        }
    }

    PKCS12 *repacked = PKCS12_create(kRepackedP12Password,
                                     friendlyName.toUtf8().constData(),
                                     privateKey,
                                     certificate,
                                     caChain,
                                     NID_pbe_WithSHA1And3_Key_TripleDES_CBC,
                                     NID_pbe_WithSHA1And3_Key_TripleDES_CBC,
                                     PKCS12_DEFAULT_ITER,
                                     PKCS12_DEFAULT_ITER,
                                     0);
    EVP_PKEY_free(privateKey);
    X509_free(certificate);
    if (caChain) sk_X509_pop_free(caChain, X509_free);

    if (!repacked) {
        qCritical() << "[IKEv2-mac] failed to repackage client p12";
        return false;
    }

    unsigned char *encoded = nullptr;
    int encodedLength = i2d_PKCS12(repacked, &encoded);
    PKCS12_free(repacked);

    if (encodedLength <= 0 || !encoded) {
        qCritical() << "[IKEv2-mac] failed to serialize repackaged client p12";
        return false;
    }

    repackedP12 = QByteArray(reinterpret_cast<const char *>(encoded), encodedLength);
    OPENSSL_free(encoded);
    return true;
}

// Removes any previously imported identity with this label from the login keychain.
void removeIdentityFromLoginKeychain(const QString &label)
{
    NSDictionary *query = @{
        (__bridge id)kSecClass : (__bridge id)kSecClassIdentity,
        (__bridge id)kSecAttrLabel : label.toNSString(),
        (__bridge id)kSecMatchLimit : (__bridge id)kSecMatchLimitAll,
    };
    SecItemDelete((__bridge CFDictionaryRef)query);
}

// Imports the client identity into the user's login keychain with an access list
// that grants this app and the VPN agent (neagent) access. Setting the ACL at
// creation time (legacy SecAccess) avoids the System keychain admin prompt and the
// partition-list authorization that cannot be satisfied on the System keychain.
bool importIdentityToLoginKeychain(const QByteArray &p12, const QString &label)
{
    SecKeychainRef loginKeychain = NULL;
    if (SecKeychainCopyDefault(&loginKeychain) != errSecSuccess || loginKeychain == NULL) {
        qCritical() << "[IKEv2-mac] cannot open the login keychain";
        return false;
    }

    CFMutableArrayRef trustedApps = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    SecTrustedApplicationRef selfApp = NULL;
    if (SecTrustedApplicationCreateFromPath(NULL, &selfApp) == errSecSuccess) {
        CFArrayAppendValue(trustedApps, selfApp);
        CFRelease(selfApp);
    }
    SecTrustedApplicationRef neAgent = NULL;
    if (SecTrustedApplicationCreateFromPath("/usr/libexec/neagent", &neAgent) == errSecSuccess) {
        CFArrayAppendValue(trustedApps, neAgent);
        CFRelease(neAgent);
    }

    SecAccessRef access = NULL;
    OSStatus accessStatus = SecAccessCreate((__bridge CFStringRef)label.toNSString(), trustedApps, &access);
    CFRelease(trustedApps);
    if (accessStatus != errSecSuccess || access == NULL) {
        qCritical() << "[IKEv2-mac] SecAccessCreate failed, status" << (int)accessStatus;
        CFRelease(loginKeychain);
        return false;
    }

    NSData *p12Data = [NSData dataWithBytes:p12.constData() length:p12.size()];
    NSDictionary *options = @{
        (__bridge id)kSecImportExportPassphrase : [NSString stringWithUTF8String:kRepackedP12Password],
        (__bridge id)kSecImportExportKeychain : (__bridge id)loginKeychain,
        (__bridge id)kSecImportExportAccess : (__bridge id)access,
    };

    CFArrayRef items = NULL;
    OSStatus importStatus = SecPKCS12Import((__bridge CFDataRef)p12Data, (__bridge CFDictionaryRef)options, &items);
    if (items) {
        CFRelease(items);
    }
    CFRelease(access);
    CFRelease(loginKeychain);

    if (importStatus != errSecSuccess) {
        qCritical() << "[IKEv2-mac] SecPKCS12Import into login keychain failed, status" << (int)importStatus;
        return false;
    }
    return true;
}

// Looks up the persistent reference of the client identity in the login keychain.
// The search is restricted to the login keychain so a stale identity that an earlier
// build left in the System keychain (same label) can never be picked instead.
NSData *copyIdentityPersistentRef(const QString &label)
{
    SecKeychainRef loginKeychain = NULL;
    SecKeychainCopyDefault(&loginKeychain);

    NSMutableDictionary *query = [@{
        (__bridge id)kSecClass : (__bridge id)kSecClassIdentity,
        (__bridge id)kSecAttrLabel : label.toNSString(),
        (__bridge id)kSecMatchLimit : (__bridge id)kSecMatchLimitOne,
        (__bridge id)kSecReturnPersistentRef : @YES,
    } mutableCopy];
    if (loginKeychain) {
        query[(__bridge id)kSecMatchSearchList] = @[ (__bridge id)loginKeychain ];
    }

    CFTypeRef persistentRef = NULL;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &persistentRef);
    if (loginKeychain) {
        CFRelease(loginKeychain);
    }

    if (status != errSecSuccess || persistentRef == NULL) {
        qDebug() << "[IKEv2-mac] client identity not present in login keychain, status" << (int)status;
        return nil;
    }

    return (NSData *)CFAutorelease(persistentRef);
}

// Runs /usr/bin/security as root through the privileged Amnezia service.
void runPrivilegedSecurity(const QString &label, const QStringList &arguments)
{
    auto process = IpcClient::CreatePrivilegedProcess();
    if (!process) {
        qCritical() << "[IKEv2-mac] privileged service is unavailable for" << label;
        return;
    }

    process->setProgram(PermittedProcess::Security);
    process->setArguments(arguments);
    process->start();

    bool started = false;
    {
        auto reply = process->waitForStarted(5000);
        reply.waitForFinished();
        started = reply.returnValue();
    }

    bool finished = false;
    {
        auto reply = process->waitForFinished(15000);
        reply.waitForFinished();
        finished = reply.returnValue();
    }

    QByteArray out;
    QByteArray err;
    {
        auto reply = process->readAllStandardOutput();
        reply.waitForFinished();
        out = reply.returnValue();
    }
    {
        auto reply = process->readAllStandardError();
        reply.waitForFinished();
        err = reply.returnValue();
    }

    qInfo() << "[IKEv2-mac]" << label << "started" << started << "finished" << finished
            << "| out:" << out.trimmed() << "| err:" << err.trimmed();
}

}  // namespace

Ikev2ProtocolMacos::Ikev2ProtocolMacos(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
    readIkev2Configuration(configuration);
}

Ikev2ProtocolMacos::~Ikev2ProtocolMacos()
{
    qInfo() << "[IKEv2-mac] ~Ikev2ProtocolMacos()";
    if (m_handshakeTimeoutTimer) {
        m_handshakeTimeoutTimer->stop();
        m_handshakeTimeoutTimer = nullptr;
    }
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
    // Already installed for this client: reuse it. Re-importing on every connect
    // churns the key, re-triggers the neagent keychain prompt and disturbs the key
    // an active (tearing-down) connection still references during a reconnect.
    if (copyIdentityPersistentRef(m_clientId) != nil) {
        qInfo() << "[IKEv2-mac] client identity already in login keychain, reusing it";
        return true;
    }

    qInfo() << "[IKEv2-mac] installing client identity into login keychain";

    QByteArray sourceP12 = QByteArray::fromBase64(m_clientCertBase64.toUtf8());
    QByteArray repackedP12;
    QByteArray caCertDer;
    if (!prepareIdentity(sourceP12, m_clientId, repackedP12, caCertDer)) {
        return false;
    }

    // Install the client identity into the user's login keychain with an ACL granting
    // neagent access. No admin rights are needed and no System keychain prompt appears.
    removeIdentityFromLoginKeychain(m_clientId);
    if (!importIdentityToLoginKeychain(repackedP12, m_clientId)) {
        return false;
    }

    // Install and trust the CA so the self-signed server certificate validates.
    if (!caCertDer.isEmpty()) {
        QTemporaryFile caFile(QDir::tempPath() + "/amnezia-ikev2-ca-XXXXXX.cer");
        caFile.setAutoRemove(false);
        if (caFile.open()) {
            const QString caPath = caFile.fileName();
            caFile.write(caCertDer);
            caFile.close();
            runPrivilegedSecurity("CA trust", { "add-trusted-cert", "-d", "-r", "trustRoot",
                                    "-k", QString::fromLatin1(kVpnSystemKeychainPath), caPath });
            QFile::remove(caPath);
        }
    } else {
        qWarning() << "[IKEv2-mac] no CA certificate in client p12; server validation may fail";
    }

    return true;
}

void Ikev2ProtocolMacos::reportError(ErrorCode code)
{
    QMetaObject::invokeMethod(this, [this, code]() { setLastError(code); }, Qt::QueuedConnection);
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
    qInfo() << "[IKEv2-mac] start() requested, host =" << m_hostName << ", clientId =" << m_clientId;

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

    NSString *nsServerAddress = m_hostName.toNSString();
    NSString *nsLocalIdentifier = m_clientId.toNSString();
    QString clientId = m_clientId;

    QPointer<Ikev2ProtocolMacos> self = this;

    dispatch_async(dispatch_get_main_queue(), ^{
      if (!self) return;
      NEVPNManager *manager = [NEVPNManager sharedManager];

      [manager loadFromPreferencesWithCompletionHandler:^(NSError *loadError) {
        if (!self) return;
        if (loadError) {
            qCritical() << "[IKEv2-mac] loading VPN preferences failed:"
                        << QString::fromNSString(loadError.localizedDescription);
            self->reportError(ErrorCode::IKEv2LoadError);
            return;
        }

        NSData *identityReference = copyIdentityPersistentRef(clientId);
        if (identityReference == nil) {
            self->reportError(ErrorCode::IKEv2ConfigError);
            return;
        }

        NEVPNProtocolIKEv2 *protocol = [[NEVPNProtocolIKEv2 alloc] init];

        protocol.serverAddress = nsServerAddress;
        protocol.remoteIdentifier = nsServerAddress;
        protocol.localIdentifier = nsLocalIdentifier;

        protocol.authenticationMethod = NEVPNIKEAuthenticationMethodCertificate;
        protocol.certificateType = NEVPNIKEv2CertificateTypeRSA;
        protocol.identityReference = identityReference;
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
          if (!self) return;
          if (firstSaveError) {
              qCritical() << "[IKEv2-mac] saving VPN preferences failed:"
                          << QString::fromNSString(firstSaveError.localizedDescription);
              self->reportError(ErrorCode::IKEv2SaveError);
              return;
          }

          [manager loadFromPreferencesWithCompletionHandler:^(NSError *reloadError) {
            if (!self) return;
            if (reloadError) {
                qCritical() << "[IKEv2-mac] reloading VPN preferences failed:"
                            << QString::fromNSString(reloadError.localizedDescription);
                self->reportError(ErrorCode::IKEv2LoadError);
                return;
            }

            [manager saveToPreferencesWithCompletionHandler:^(NSError *resaveError) {
              if (!self) return;
              if (resaveError) {
                  qCritical() << "[IKEv2-mac] re-saving VPN preferences failed:"
                              << QString::fromNSString(resaveError.localizedDescription);
                  self->reportError(ErrorCode::IKEv2SaveError);
                  return;
              }

              self->removeStatusObserver();
              self->m_statusObserver = (void *)[[NSNotificationCenter defaultCenter]
                      addObserverForName:NEVPNStatusDidChangeNotification
                                  object:manager.connection
                                   queue:nil
                              usingBlock:^(NSNotification *notification) {
                                if (!self) return;
                                NEVPNConnection *connection = notification.object;
                                int rawStatus = (int)connection.status;
                                QMetaObject::invokeMethod(
                                        self, [self, rawStatus]() { if (self) self->handleStatusChange(rawStatus); },
                                        Qt::QueuedConnection);
                              }];

              // If a previous connection is still active or tearing down, stop it first
              // and start only once it reaches Disconnected. Otherwise our fresh start
              // races with the old teardown and gets dropped immediately.
              NEVPNStatus current = manager.connection.status;
              if (current == NEVPNStatusDisconnected || current == NEVPNStatusInvalid) {
                  qInfo() << "[IKEv2-mac] preferences saved, connection is"
                          << vpnStatusName(current) << "- starting tunnel now";
                  self->m_startWhenDisconnected = false;
                  self->startTunnelNow();
              } else {
                  qInfo() << "[IKEv2-mac] preferences saved, connection is"
                          << vpnStatusName(current) << "- waiting for it to disconnect first";
                  self->m_startWhenDisconnected = true;
                  [manager.connection stopVPNTunnel];
              }
            }];
          }];
        }];
      }];
    });

    return ErrorCode::NoError;
}

void Ikev2ProtocolMacos::stop()
{
    qInfo() << "[IKEv2-mac] stop() requested";

    stopHandshakeTimeoutTimer();
    m_startWhenDisconnected = false;

    // stop() runs synchronously because VpnConnection destroys this object right after
    // it returns. Any async work (dispatch_async) would be dropped when the object dies,
    // leaving the tunnel up and the UI stuck on "Disconnecting".
    NEVPNManager *manager = [NEVPNManager sharedManager];
    NEVPNStatus status = manager.connection.status;
    qInfo() << "[IKEv2-mac] stop(): current NEVPNStatus =" << (int)status;

    removeStatusObserver();

    if (status != NEVPNStatusDisconnected && status != NEVPNStatusInvalid) {
        [manager.connection stopVPNTunnel];
        qInfo() << "[IKEv2-mac] stop(): stopVPNTunnel issued";
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

void Ikev2ProtocolMacos::startTunnelNow()
{
    qInfo() << "[IKEv2-mac] startVPNTunnel";
    NEVPNManager *manager = [NEVPNManager sharedManager];
    NSError *startError = nil;
    [manager.connection startVPNTunnelAndReturnError:&startError];
    if (startError) {
        qCritical() << "[IKEv2-mac] starting the tunnel failed:"
                    << QString::fromNSString(startError.localizedDescription);
        reportError(ErrorCode::IKEv2ConnectError);
    } else {
        QMetaObject::invokeMethod(this, [this]() { startHandshakeTimeoutTimer(); }, Qt::QueuedConnection);
    }
}

void Ikev2ProtocolMacos::handleStatusChange(int rawStatus)
{
    NEVPNStatus vpnStatus = static_cast<NEVPNStatus>(rawStatus);
    const Vpn::ConnectionState currentState = connectionState();

    qInfo() << "[IKEv2-mac] status ->" << vpnStatusName(rawStatus)
            << "| uiState:" << textConnectionState()
            << "| lastStatus:" << vpnStatusName(m_lastVpnStatus)
            << "| waitingToStart:" << m_startWhenDisconnected;

    // While waiting for a previous connection to finish tearing down, swallow its
    // teardown events and launch our tunnel only once it is fully Disconnected.
    if (m_startWhenDisconnected) {
        if (vpnStatus == NEVPNStatusDisconnecting) {
            return;
        }
        if (vpnStatus == NEVPNStatusDisconnected) {
            m_startWhenDisconnected = false;
            m_lastVpnStatus = NEVPNStatusDisconnected;
            QPointer<Ikev2ProtocolMacos> self = this;
            dispatch_async(dispatch_get_main_queue(), ^{
              if (self) self->startTunnelNow();
            });
            return;
        }
    }

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
            dispatch_async(dispatch_get_main_queue(), ^{
              [[NEVPNManager sharedManager].connection stopVPNTunnel];
            });
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
