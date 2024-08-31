#include "ikev2_vpn_protocol_mac.h"



#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#import <NetworkExtension/NetworkExtension.h>
#import <Foundation/Foundation.h>
#include <QWaitCondition>

#include <openssl/bio.h>
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <sys/sysctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>

static NSString * const IKEv1ServiceName = @"AmneziaVPN";
static NSString * const IKEv2ServiceName = @"AmneziaVPN IKEv2";

static Ikev2Protocol* self = nullptr;


Ikev2Protocol::Ikev2Protocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    qDebug() << "IpsecProtocol::IpsecProtocol()";
    self = this;
    readIkev2Configuration(configuration);
}

Ikev2Protocol::~Ikev2Protocol()
{
    qDebug() << "IpsecProtocol::~IpsecProtocol()";
    disconnect_vpn();
    Ikev2Protocol::stop();
}

void Ikev2Protocol::stop()
{
    setConnectionState(Vpn::ConnectionState::Disconnected);
    qDebug() << "IpsecProtocol::stop()";
}


void Ikev2Protocol::readIkev2Configuration(const QJsonObject &configuration)
{
    qDebug() << "IpsecProtocol::readIkev2Configuration";
    QJsonObject ikev2_data = configuration.value(ProtocolProps::key_proto_config_data(Proto::Ikev2)).toObject();
    m_config = QJsonDocument::fromJson(ikev2_data.value(config_key::config).toString().toUtf8()).object();
}

CFDataRef CreatePersistentRefForIdentity(SecIdentityRef identity)
{
    CFTypeRef  persistent_ref = NULL;
    const void *keys[] =   { kSecReturnPersistentRef, kSecValueRef };
    const void *values[] = { kCFBooleanTrue,          identity };
    CFDictionaryRef dict = CFDictionaryCreate(NULL, keys, values,
                                              sizeof(keys) / sizeof(*keys), NULL, NULL);
    
    
    if (SecItemCopyMatching(dict, &persistent_ref) != 0) {
        SecItemAdd(dict, &persistent_ref);
    }
    
    if (dict)
        CFRelease(dict);
    
    return (CFDataRef)persistent_ref;
}


ErrorCode Ikev2Protocol::start()
{

    qDebug() << "IpsecProtocol::start";
    
    static QMutex mutexLocal;
    mutexLocal.lock();
    
    setConnectionState(Vpn::ConnectionState::Disconnected);
    NEVPNManager *manager = [NEVPNManager sharedManager];

    NSString *nsUsername = m_config.value(amnezia::config_key::hostName).toString().toNSString();
    NSString *nsIp = m_config.value(amnezia::config_key::hostName).toString().toNSString();
    NSString *nsRemoteId = m_config.value(amnezia::config_key::hostName).toString().toNSString();

    [manager loadFromPreferencesWithCompletionHandler:^(NSError *err)
    {
        mutexLocal.lock();

        if (err)
        {
            qDebug() << "First load vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
            setConnectionState(Vpn::ConnectionState::Disconnected);
            mutexLocal.unlock();
        }
        else
        {
            
            NSData *output = NULL;

            BIO *ibio, *obio = NULL;
            BUF_MEM *bptr;


            STACK_OF(X509) *certstack = sk_X509_new_null();
            BIO *p12 = BIO_new(BIO_s_mem());

            EVP_PKEY *pkey;
            X509 *cert;

            BIO_write(p12, QByteArray::fromBase64(m_config[config_key::cert].toString().toUtf8()),
                      QByteArray::fromBase64(m_config[config_key::cert].toString().toUtf8()).size());

            PKCS12 *pkcs12 = d2i_PKCS12_bio(p12, NULL);
            PKCS12_parse(pkcs12, m_config[config_key::password].toString().toStdString().c_str(), &pkey, &cert, &certstack);
            
            // We output everything in PEM
            obio = BIO_new(BIO_s_mem());

            // TODO: support protecting the private key with a PEM passphrase
            if (pkey)
              {
                PEM_write_bio_PrivateKey(obio, pkey, NULL, NULL, 0, NULL, NULL);
              }

            if (cert)
              {
                PEM_write_bio_X509(obio, cert);
              }

            if (certstack && sk_X509_num(certstack))
              {
                for (int i = 0; i < sk_X509_num(certstack); i++)
                  PEM_write_bio_X509_AUX(obio, sk_X509_value(certstack, i));
              }

            BIO_get_mem_ptr(obio, &bptr);

            output = [NSData dataWithBytes: bptr->data  length: bptr->length];
            
            NSData *PKCS12Data = [[NSData alloc] initWithBase64EncodedString:m_config[config_key::cert].toString().toNSString() options:0] ;
            
            CFArrayRef items = CFArrayCreate(NULL, 0, 0, NULL);
            OSStatus ret = SecPKCS12Import(
                                           (__bridge CFDataRef)output,
                                           (__bridge CFDictionaryRef)@{(id)kSecImportExportPassphrase:@""},
                                           &items);
            
            if (ret != errSecSuccess) {
                qDebug() << "import err ret " << ret;
            }
            
            NSDictionary *firstItem = [(__bridge_transfer NSArray *)items firstObject];
            SecIdentityRef identity = (__bridge SecIdentityRef)(firstItem[(__bridge id)kSecImportItemIdentity]);

            NEVPNProtocolIKEv2 *protocol = [[NEVPNProtocolIKEv2 alloc] init];
            protocol.serverAddress = nsIp;
            protocol.certificateType = NEVPNIKEv2CertificateTypeRSA;
            
            protocol.remoteIdentifier = m_config.value(amnezia::config_key::hostName).toString().toNSString();
            
            protocol.authenticationMethod = NEVPNIKEAuthenticationMethodCertificate;
            protocol.identityReference = (__bridge NSData *)CreatePersistentRefForIdentity(identity);
            
            protocol.useExtendedAuthentication = YES;
            protocol.enablePFS = YES;
            
            protocol.IKESecurityAssociationParameters.encryptionAlgorithm = NEVPNIKEv2EncryptionAlgorithmAES256;
            protocol.IKESecurityAssociationParameters.diffieHellmanGroup = NEVPNIKEv2DiffieHellmanGroup19;
            protocol.IKESecurityAssociationParameters.integrityAlgorithm = NEVPNIKEv2IntegrityAlgorithmSHA256;
            protocol.IKESecurityAssociationParameters.lifetimeMinutes = 1440;

            protocol.childSecurityAssociationParameters.encryptionAlgorithm = NEVPNIKEv2EncryptionAlgorithmAES256;
            protocol.childSecurityAssociationParameters.diffieHellmanGroup = NEVPNIKEv2DiffieHellmanGroup19;
            protocol.childSecurityAssociationParameters.integrityAlgorithm = NEVPNIKEv2IntegrityAlgorithmSHA256;
            protocol.childSecurityAssociationParameters.lifetimeMinutes = 1440;

            [manager setEnabled:YES];
            [manager setProtocolConfiguration:(protocol)];
            [manager setOnDemandEnabled:NO];
            [manager setLocalizedDescription:@"Amnezia VPN"];

            NSString *strProtocol = [NSString stringWithFormat:@"{Protocol: %@", protocol];
            qDebug() << QString::fromNSString(strProtocol);

            // do config stuff
            [manager saveToPreferencesWithCompletionHandler:^(NSError *err)
            {
                if (err)
                {
                    qDebug() << "First save vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
                    setConnectionState(Vpn::ConnectionState::Disconnected);
                    mutexLocal.unlock();
                }
                else
                {
                    // load and save preferences again, otherwise Mac bug (https://forums.developer.apple.com/thread/25928)
                    [manager loadFromPreferencesWithCompletionHandler:^(NSError *err)
                    {
                        if (err)
                        {
                            qDebug() << "Second load vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
                            setConnectionState(Vpn::ConnectionState::Disconnected);
                            mutexLocal.unlock();
                        }
                        else
                        {
                            [manager saveToPreferencesWithCompletionHandler:^(NSError *err)
                            {
                                if (err)
                                {
                                    qDebug() << "Second Save vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
                                    setConnectionState(Vpn::ConnectionState::Disconnected);
                                    mutexLocal.unlock();
                                }
                                else
                                {
                                    notificationId_ = [[NSNotificationCenter defaultCenter] addObserverForName: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection queue: nil usingBlock: ^ (NSNotification *notification)
                                    {
                                        this->handleNotification(notification);
                                    }];

                                    qDebug() << "NEVPNConnection current status:" << (int)manager.connection.status;

                                    NSError *startError;
                                    [manager.connection startVPNTunnelAndReturnError:&startError];
                                    if (startError)
                                    {
                                        qDebug() << "Error starting ikev2 connection:" << QString::fromNSString(startError.localizedDescription);
                                        [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
                                        setConnectionState(Vpn::ConnectionState::Disconnected);
                                    }
                                    mutexLocal.unlock();
                                }
                            }];
                        }
                    }];
                }
            }];
        }
    }];

   // waitConditionLocal.wait(&mutexLocal);
    mutexLocal.unlock();

    setConnectionState(Vpn::ConnectionState::Connected);
    return ErrorCode::NoError;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::create_new_vpn(const QString & vpn_name,
                                   const QString & serv_addr){
   qDebug() << "Ikev2Protocol::create_new_vpn()";
   return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::delete_vpn_connection(const QString &vpn_name){

    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::connect_to_vpn(const QString & vpn_name){
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::disconnect_vpn() {

    QMutexLocker locker(&mutex_);

    NEVPNManager *manager = [NEVPNManager sharedManager];

    // #713: If user had started connecting to IKev2 on Mac and quickly started after this connecting to Wireguard

    //       then manager.connection.status doesn't have time to change to NEVPNStatusConnecting
    //       and remains NEVPNStatusDisconnected as it was before connection tries.
    //       Then we should check below isConnectingStateReachedAfterStartingConnection_ flag to be sure that connecting started.
    //       Without this check we will start connecting to the Wireguard when IKEv2 connecting process hasn't finished yet.
    if (manager.connection.status == NEVPNStatusDisconnected && isConnectingStateReachedAfterStartingConnection_)
    {
        [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
        setConnectionState(Vpn::ConnectionState::Disconnected);
    }
    else
    {
        [manager.connection stopVPNTunnel];
    }

    return true;
}


void Ikev2Protocol::closeWindscribeActiveConnection()
{
    static QWaitCondition waitCondition;
    static QMutex mutex;

    mutex.lock();

    NEVPNManager *manager = [NEVPNManager sharedManager];
    if (manager)
    {
        [manager loadFromPreferencesWithCompletionHandler:^(NSError *err)
        {
            mutex.lock();
            if (!err)
            {
                NEVPNConnection * connection = [manager connection];
                if (connection.status == NEVPNStatusConnected || connection.status == NEVPNStatusConnecting)
                {
                    if ([manager.localizedDescription isEqualToString:@"Amnezia VPN"] == YES)
                    {
                        qDebug() << "Previous IKEv2 connection is active. Stop it.";
                        [connection stopVPNTunnel];
                    }
                }
            }
            waitCondition.wakeAll();
            mutex.unlock();
        }];
    }
    waitCondition.wait(&mutex);
    mutex.unlock();
}

void Ikev2Protocol::handleNotificationImpl(int status)
{
    QMutexLocker locker(&mutex_);

    NEVPNManager *manager = [NEVPNManager sharedManager];

    if (status == NEVPNStatusInvalid)
    {
        qDebug() << "Connection status changed: NEVPNStatusInvalid";
        [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
        setConnectionState(Vpn::ConnectionState::Disconnected);

    }
    else if (status == NEVPNStatusDisconnected)
    {
        qDebug() << "Connection status changed: NEVPNStatusDisconnected";

        if (state_ == STATE_DISCONNECTING_ANY_ERROR)
        {
            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
           // state_ = STATE_DISCONNECTED;
           // emit error(IKEV_FAILED_TO_CONNECT);
            setConnectionState(Vpn::ConnectionState::Disconnected);
        }
        else if (state_ != STATE_DISCONNECTED)
        {

            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
           // state_ = STATE_DISCONNECTED;
            setConnectionState(Vpn::ConnectionState::Disconnected);
        }
    }
    else if (status == NEVPNStatusConnecting)
    {
        isConnectingStateReachedAfterStartingConnection_ = true;
        qDebug() << "Connection status changed: NEVPNStatusConnecting";
    }
    else if (status == NEVPNStatusConnected)
    {
        if (!overrideDnsIp_.isEmpty()) {
            if (!setCustomDns(overrideDnsIp_)) {
                qDebug() << "Failed to set custom DNS ip for ikev2";
            }
        }

        qDebug() << "Connection status changed: NEVPNStatusConnected";

        setConnectionState(Vpn::ConnectionState::Connected);
        // note: route gateway not used for ikev2 in AdapterGatewayInfo
    //    AdapterGatewayInfo cai;
    //    ipsecAdapterName_ = NetworkUtils_mac::lastConnectedNetworkInterfaceName();
    //    cai.setAdapterName(ipsecAdapterName_);
    //    cai.setAdapterIp(NetworkUtils_mac::ipAddressByInterfaceName(ipsecAdapterName_));
        //cai.setDnsServers(NetworkUtils_mac::getDnsServersForInterface(ipsecAdapterName_));
    }
    else if (status == NEVPNStatusReasserting)
    {
        qDebug() << "Connection status changed: NEVPNStatusReasserting";
        setConnectionState(Vpn::ConnectionState::Connecting);
    }
    else if (status == NEVPNStatusDisconnecting)
    {
        qDebug() << "Connection status changed: NEVPNStatusDisconnecting";
        setConnectionState(Vpn::ConnectionState::Disconnecting);
  /*      if (state_ == STATE_START_CONNECT)
        {
            QMap<time_t, QString> logs = networkExtensionLog_.collectNext();
            for (QMap<time_t, QString>::iterator it = logs.begin(); it != logs.end(); ++it)
            {
                qDebug() << it.value();
            }
            if (isSocketError(logs))
            {
                state_ = STATE_DISCONNECTING_ANY_ERROR;
            }
            else
            {
                if (isFailedAuthError(logs))
                {
                    state_ = STATE_DISCONNECTING_AUTH_ERROR;
                }
                else
                {
                    state_ = STATE_DISCONNECTING_ANY_ERROR;
                }
            }
        }*/
    }

    prevConnectionStatus_ = status;
    isPrevConnectionStatusInitialized_ = true;
}


void Ikev2Protocol::handleNotification(void *notification)
{
    QMutexLocker locker(&mutex_);
    NSNotification *nsNotification = (NSNotification *)notification;
    NEVPNConnection *connection = nsNotification.object;
    QMetaObject::invokeMethod(this, "handleNotificationImpl", Q_ARG(int, (int)connection.status));
}

bool Ikev2Protocol::isFailedAuthError(QMap<time_t, QString> &logs)
{
    for (QMap<time_t, QString>::iterator it = logs.begin(); it != logs.end(); ++it)
    {
        if (it.value().contains("Failed", Qt::CaseInsensitive) && it.value().contains("IKE", Qt::CaseInsensitive) && it.value().contains("Auth", Qt::CaseInsensitive))
        {
            if (!(it.value().contains("Failed", Qt::CaseInsensitive) && it.value().contains("IKEv2 socket", Qt::CaseInsensitive)))
            {
                return true;
            }
        }
    }
    return false;
}

bool Ikev2Protocol::isSocketError(QMap<time_t, QString> &logs)
{
    for (QMap<time_t, QString>::iterator it = logs.begin(); it != logs.end(); ++it)
    {
        if (it.value().contains("Failed", Qt::CaseInsensitive) && it.value().contains("initialize", Qt::CaseInsensitive) && it.value().contains("socket", Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
}

bool Ikev2Protocol::setCustomDns(const QString &overrideDnsIpAddress)
{
    // get list of entries of interest
 //   QStringList networkServices = NetworkUtils_mac::getListOfDnsNetworkServiceEntries();

    // filter list to only ikev2 entries
    QStringList dnsNetworkServices;
  //  for (const QString &service : networkServices)
  //      if (MacUtils::dynamicStoreEntryHasKey(service, "ConfirmedServiceID"))
  //          dnsNetworkServices.append(service);

    qDebug() << "Applying custom 'while connected' DNS change to network services: " << dnsNetworkServices;

    if (dnsNetworkServices.isEmpty()) {
        qDebug() << "No network services to configure 'while connected' DNS";
        return false;
    }

    // change DNS on each entry
    bool successAll = true;
    for (const QString &service : dnsNetworkServices) {
 //       if (!helper_->setDnsOfDynamicStoreEntry(overrideDnsIpAddress, service)) {
 //           successAll = false;
 //           qDebug() << "Failed to set network service DNS: " << service;
 //           break;
 //       }
    }

    return successAll;
}


