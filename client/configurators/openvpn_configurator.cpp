#include "openvpn_configurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/networkUtilities.h"
#include "core/scripts_registry.h"
#include "settings.h"
#include "utilities.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

OpenVpnConfigurator::OpenVpnConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                         QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const ServerCredentials &serverCredentials,
                                                                              const ContainerConfig &containerConfig, ErrorCode &errorCode)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = serverCredentials.hostName;

    if (connData.privKey.isEmpty() || connData.request.isEmpty()) {
        errorCode = ErrorCode::OpenSslFailed;
        return connData;
    }

    QString reqFileName = QString("%1/%2.req").arg(amnezia::protocols::openvpn::clientsDirPath).arg(connData.clientId);

    errorCode =
            m_serverController->uploadTextFileToContainer(containerConfig.containerType, serverCredentials, connData.request, reqFileName);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    errorCode = signCert(serverCredentials, containerConfig, connData.clientId);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.caCert = m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials,
                                                                   amnezia::protocols::openvpn::caCertPath, errorCode);
    connData.clientCert = m_serverController->getTextFileFromContainer(
            containerConfig.containerType, serverCredentials,
            QString("%1/%2.crt").arg(amnezia::protocols::openvpn::clientCertPath).arg(connData.clientId), errorCode);

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.taKey = m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials,
                                                                  amnezia::protocols::openvpn::taKeyPath, errorCode);

    if (connData.caCert.isEmpty() || connData.clientCert.isEmpty() || connData.taKey.isEmpty()) {
        errorCode = ErrorCode::SshScpFailureError;
    }

    return connData;
}

QSharedPointer<ProtocolConfig> OpenVpnConfigurator::createConfig(const ServerCredentials &serverCredentials,
                                                                 const ContainerConfig &containerConfig, ErrorCode &errorCode)
{
    QString config = m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::openvpn_template, containerConfig.containerType),
                                                     m_serverController->genVarsForScript(containerConfig, serverCredentials));

    ConnectionData connData = prepareOpenVpnConfig(serverCredentials, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    config.replace("$OPENVPN_CA_CERT", connData.caCert);
    config.replace("$OPENVPN_CLIENT_CERT", connData.clientCert);
    config.replace("$OPENVPN_PRIV_KEY", connData.privKey);

    if (config.contains("$OPENVPN_TA_KEY")) {
        config.replace("$OPENVPN_TA_KEY", connData.taKey);
    } else {
        config.replace("<tls-auth>", "");
        config.replace("</tls-auth>", "");
    }

#ifndef MZ_WINDOWS
    config.replace("block-outside-dns", "");
#endif

    auto baseProtocolConfig = qSharedPointerCast<OpenVpnProtocolConfig>(
            containerConfig.protocolConfigs.value(ProtocolProps::protoToString(Proto::OpenVpn)));
    auto openVpnConfig = QSharedPointer<OpenVpnProtocolConfig>::create(*baseProtocolConfig);

    openVpnConfig->clientProtocolConfig.isEmpty = false;
    openVpnConfig->clientProtocolConfig.clientId = connData.clientId;
    openVpnConfig->clientProtocolConfig.nativeConfig = config;

    return openVpnConfig;
}

QSharedPointer<ProtocolConfig> OpenVpnConfigurator::processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                                                   QSharedPointer<ProtocolConfig> protocolConfig)
{
    if (!protocolConfig) {
        return nullptr;
    }

    auto openVpnConfig = qSharedPointerCast<OpenVpnProtocolConfig>(protocolConfig);
    if (!openVpnConfig) {
        return protocolConfig;
    }

    QString config = openVpnConfig->clientProtocolConfig.nativeConfig;

    config.replace("$PRIMARY_DNS", dns.first);
    config.replace("$SECONDARY_DNS", dns.second);

    if (!isApiConfig) {
        QRegularExpression regex("redirect-gateway.*");
        config.replace(regex, "");

        // We don't use secondary DNS if primary DNS is AmneziaDNS
        if (dns.first.contains(protocols::dns::amneziaDnsIp)) {
            QRegularExpression dnsRegex("dhcp-option DNS " + dns.second);
            config.replace(dnsRegex, "");
        }

        if (!m_settings->isSitesSplitTunnelingEnabled()) {
            config.append("\nredirect-gateway def1 ipv6 bypass-dhcp\n");
            config.append("block-ipv6\n");
        } else if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {

            // no redirect-gateway
        } else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
            config.append("\nredirect-gateway ipv6 !ipv4 bypass-dhcp\n");
            // Prevent ipv6 leak
#endif
            config.append("block-ipv6\n");
        }
    }

#ifndef MZ_WINDOWS
    config.replace("block-outside-dns", "");
#endif

#if (defined(MZ_MACOS) || defined(MZ_LINUX))
    QString dnsConf = QString("\nscript-security 2\n"
                              "up %1/update-resolv-conf.sh\n"
                              "down %1/update-resolv-conf.sh\n")
                              .arg(qApp->applicationDirPath());

    config.append(dnsConf);
#endif

    openVpnConfig->clientProtocolConfig.nativeConfig = config;
    return openVpnConfig;
}

QSharedPointer<ProtocolConfig> OpenVpnConfigurator::processConfigWithExportSettings(const QPair<QString, QString> &dns,
                                                                                    QSharedPointer<ProtocolConfig> protocolConfig)
{
    if (!protocolConfig) {
        return nullptr;
    }

    auto openVpnConfig = qSharedPointerCast<OpenVpnProtocolConfig>(protocolConfig);
    if (!openVpnConfig) {
        return protocolConfig;
    }

    QString config = openVpnConfig->clientProtocolConfig.nativeConfig;

    config.replace("$PRIMARY_DNS", dns.first);
    config.replace("$SECONDARY_DNS", dns.second);

    QRegularExpression regex("redirect-gateway.*");
    config.replace(regex, "");

    // We don't use secondary DNS if primary DNS is AmneziaDNS
    if (dns.first.contains(protocols::dns::amneziaDnsIp)) {
        QRegularExpression dnsRegex("dhcp-option DNS " + dns.second);
        config.replace(dnsRegex, "");
    }

    config.append("\nredirect-gateway def1 ipv6 bypass-dhcp\n");

    // Prevent ipv6 leak
    config.append("block-ipv6\n");

    // remove block-outside-dns for all exported configs
    config.replace("block-outside-dns", "");

    openVpnConfig->clientProtocolConfig.nativeConfig = config;
    return openVpnConfig;
}

ErrorCode OpenVpnConfigurator::signCert(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig, QString clientId)
{
    QString script_import = QString("sudo docker exec -i %1 bash -c \"cd /opt/amnezia/openvpn && "
                                    "easyrsa import-req %2/%3.req %3\"")
                                    .arg(containerConfig.containerName)
                                    .arg(amnezia::protocols::openvpn::clientsDirPath)
                                    .arg(clientId);

    QString script_sign = QString("sudo docker exec -i %1 bash -c \"export EASYRSA_BATCH=1; cd /opt/amnezia/openvpn && "
                                  "easyrsa sign-req client %2\"")
                                  .arg(containerConfig.containerName)
                                  .arg(clientId);

    QStringList scriptList { script_import, script_sign };
    QString script =
            m_serverController->replaceVars(scriptList.join("\n"), m_serverController->genVarsForScript(containerConfig, serverCredentials));

    return m_serverController->runScript(serverCredentials, script);
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::createCertRequest()
{
    ConnectionData connData;
    connData.clientId = Utils::getRandomString(32);

    int ret = 0;
    int nVersion = 1;

    QByteArray clientIdUtf8 = connData.clientId.toUtf8();

    EVP_PKEY *pKey = EVP_PKEY_new();
    q_check_ptr(pKey);
    RSA *rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
    q_check_ptr(rsa);
    EVP_PKEY_assign_RSA(pKey, rsa);

    // 2. set version of x509 req
    X509_REQ *x509_req = X509_REQ_new();
    ret = X509_REQ_set_version(x509_req, nVersion);
    if (ret != 1) {
        qWarning() << "Could not get X509!";
        X509_REQ_free(x509_req);
        EVP_PKEY_free(pKey);
        return connData;
    }

    // 3. set subject of x509 req
    X509_NAME *x509_name = X509_REQ_get_subject_name(x509_req);

    X509_NAME_add_entry_by_txt(x509_name, "C", MBSTRING_ASC, (unsigned char *)"ORG", -1, -1, 0);
    X509_NAME_add_entry_by_txt(x509_name, "O", MBSTRING_ASC, (unsigned char *)"", -1, -1, 0);
    X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC, reinterpret_cast<unsigned char const *>(clientIdUtf8.data()),
                               clientIdUtf8.size(), -1, 0);

    // 4. set public key of x509 req
    ret = X509_REQ_set_pubkey(x509_req, pKey);
    if (ret != 1) {
        qWarning() << "Could not set pubkey!";
        X509_REQ_free(x509_req);
        EVP_PKEY_free(pKey);
        return connData;
    }

    // 5. set sign key of x509 req
    ret = X509_REQ_sign(x509_req, pKey, EVP_sha256()); // return x509_req->signature->length
    if (ret <= 0) {
        qWarning() << "Could not sign request!";
        X509_REQ_free(x509_req);
        EVP_PKEY_free(pKey);
        return connData;
    }

    // save private key
    BIO *bp_private = BIO_new(BIO_s_mem());
    q_check_ptr(bp_private);
    if (PEM_write_bio_PrivateKey(bp_private, pKey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        qFatal("PEM_write_bio_PrivateKey");
        EVP_PKEY_free(pKey);
        BIO_free_all(bp_private);
        X509_REQ_free(x509_req);
        return connData;
    }

    const char *buffer = nullptr;
    size_t size = BIO_get_mem_data(bp_private, &buffer);
    q_check_ptr(buffer);
    connData.privKey = QByteArray(buffer, size);
    if (connData.privKey.isEmpty()) {
        qFatal("Failed to generate a random private key");
        EVP_PKEY_free(pKey);
        BIO_free_all(bp_private);
        X509_REQ_free(x509_req);
        return connData;
    }
    BIO_free_all(bp_private);

    // save req
    BIO *bio_req = BIO_new(BIO_s_mem());
    PEM_write_bio_X509_REQ(bio_req, x509_req);

    BUF_MEM *bio_buf;
    BIO_get_mem_ptr(bio_req, &bio_buf);
    connData.request = QByteArray(bio_buf->data, bio_buf->length);
    BIO_free(bio_req);

    EVP_PKEY_free(pKey); // this will also free the rsa key

    return connData;
}
