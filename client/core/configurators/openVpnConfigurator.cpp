#include "openVpnConfigurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/utilities.h"
#include "core/models/protocols/openVpnProtocolConfig.h"

using namespace amnezia;

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>


OpenVpnConfigurator::OpenVpnConfigurator(SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(sshSession, parent)
{
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const ServerCredentials &credentials,
                                                                              DockerContainer container, 
                                                                              const DnsSettings &dnsSettings,
                                                                              ErrorCode &errorCode)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = credentials.hostName;

    if (connData.privKey.isEmpty() || connData.request.isEmpty()) {
        errorCode = ErrorCode::OpenSslFailed;
        return connData;
    }

    QString reqFileName = QString("%1/%2.req").arg(amnezia::protocols::openvpn::clientsDirPath).arg(connData.clientId);

    errorCode = m_sshSession->uploadTextFileToContainer(container, credentials, connData.request, reqFileName);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    errorCode = signCert(container, credentials, dnsSettings, connData.clientId);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.caCert =
            m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::openvpn::caCertPath, errorCode);
    connData.clientCert = m_sshSession->getTextFileFromContainer(
            container, credentials, QString("%1/%2.crt").arg(amnezia::protocols::openvpn::clientCertPath).arg(connData.clientId), errorCode);

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.taKey = m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::openvpn::taKeyPath, errorCode);

    if (connData.caCert.isEmpty() || connData.clientCert.isEmpty() || connData.taKey.isEmpty()) {
        errorCode = ErrorCode::SshScpFailureError;
    }

    return connData;
}

ProtocolConfig OpenVpnConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                  const ContainerConfig &containerConfig,
                                                  const DnsSettings &dnsSettings,
                                                  ErrorCode &errorCode)
{
    const OpenVpnServerConfig* serverConfig = nullptr;
    if (auto* openVpnProtocolConfig = containerConfig.getOpenVpnProtocolConfig()) {
        serverConfig = &openVpnProtocolConfig->serverConfig;
    }
    
    amnezia::ScriptVars vars = amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns);
    vars.append(amnezia::genProtocolVarsForContainer(container, containerConfig));
    QString config = m_sshSession->replaceVars(amnezia::scriptData(ProtocolScriptType::openvpn_template, container), vars);

    ConnectionData connData = prepareOpenVpnConfig(credentials, container, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return OpenVpnProtocolConfig{};
    }

    auto sanitizeStaticKey = [](const QString &key) {
        QStringList lines = key.split('\n');
        QStringList filtered;
        filtered.reserve(lines.size());
        for (const QString &line : lines) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith('#')) {
                continue;
            }
            filtered.append(line);
        }
        QString result = filtered.join('\n');
        if (!result.endsWith('\n')) {
            result.append('\n');
        }
        return result;
    };

    config.replace("$OPENVPN_CA_CERT", connData.caCert);
    config.replace("$OPENVPN_CLIENT_CERT", connData.clientCert);
    config.replace("$OPENVPN_PRIV_KEY", connData.privKey);

    if (config.contains("$OPENVPN_TA_KEY")) {
        config.replace("$OPENVPN_TA_KEY", sanitizeStaticKey(connData.taKey));
    } else {
        config.replace("<tls-auth>", "");
        config.replace("</tls-auth>", "");
    }

#ifndef MZ_WINDOWS
    config.replace("block-outside-dns", "");
#endif

    OpenVpnProtocolConfig protocolConfig;
    if (serverConfig) {
        protocolConfig.serverConfig = *serverConfig;
    }
    
    OpenVpnClientConfig clientConfig;
    clientConfig.nativeConfig = config;
    clientConfig.clientId = connData.clientId;
    clientConfig.blockOutsideDns = false;
    
    protocolConfig.setClientConfig(clientConfig);
    
    return protocolConfig;
}

ProtocolConfig OpenVpnConfigurator::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                   ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);

    QString config = protocolConfig.nativeConfig();

    if (!settings.isApiConfig) {
        QRegularExpression regex("redirect-gateway.*");
        config.replace(regex, "");

        if (settings.dns.primaryDns.contains(protocols::dns::amneziaDnsIp)) {
            QRegularExpression dnsRegex("dhcp-option DNS " + settings.dns.secondaryDns);
            config.replace(dnsRegex, "");
        }

        if (!settings.splitTunneling.isSitesSplitTunnelingEnabled) {
            config.append("\nredirect-gateway def1 ipv6 bypass-dhcp\n");
            config.append("block-ipv6\n");
        } else if (settings.splitTunneling.routeMode == RouteMode::VpnOnlyForwardSites) {
            // no redirect-gateway
        } else if (settings.splitTunneling.routeMode == RouteMode::VpnAllExceptSites) {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
            config.append("\nredirect-gateway ipv6 !ipv4 bypass-dhcp\n");
#endif
            config.append("block-ipv6\n");
        }
    }

#ifndef MZ_WINDOWS
    config.replace("block-outside-dns", "");
#endif

#if (defined(MZ_MACOS) || defined(MZ_LINUX))
    config.append(QString("\nscript-security 2\n"
                         "up %1/update-resolv-conf.sh\n"
                         "down %1/update-resolv-conf.sh\n")
                  .arg(qApp->applicationDirPath()));
#endif

    protocolConfig.setNativeConfig(config);
    return protocolConfig;
}

ProtocolConfig OpenVpnConfigurator::processConfigWithExportSettings(const ExportSettings &settings,
                                                                    ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);

    QString config = protocolConfig.nativeConfig();

    QRegularExpression regex("redirect-gateway.*");
    config.replace(regex, "");

    if (settings.dns.primaryDns.contains(protocols::dns::amneziaDnsIp)) {
        QRegularExpression dnsRegex("dhcp-option DNS " + settings.dns.secondaryDns);
        config.replace(dnsRegex, "");
    }

    config.append("\nredirect-gateway def1 ipv6 bypass-dhcp\n");
    config.append("block-ipv6\n");
    config.replace("block-outside-dns", "");

    protocolConfig.setNativeConfig(config);
    return protocolConfig;
}

ErrorCode OpenVpnConfigurator::signCert(DockerContainer container, const ServerCredentials &credentials, 
                                        const DnsSettings &dnsSettings, QString clientId)
{
    QString script_import = QString("sudo docker exec -i %1 bash -c \"cd /opt/amnezia/openvpn && "
                                    "easyrsa import-req %2/%3.req %3\"")
                                    .arg(ContainerUtils::containerToString(container))
                                    .arg(amnezia::protocols::openvpn::clientsDirPath)
                                    .arg(clientId);

    QString script_sign = QString("sudo docker exec -i %1 bash -c \"export EASYRSA_BATCH=1; cd /opt/amnezia/openvpn && "
                                  "easyrsa sign-req client %2\"")
                                  .arg(ContainerUtils::containerToString(container))
                                  .arg(clientId);

    QStringList scriptList { script_import, script_sign };
    QString script = m_sshSession->replaceVars(scriptList.join("\n"), amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns));

    return m_sshSession->runScript(credentials, script);
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::createCertRequest()
{
    ConnectionData connData;
    connData.clientId = Utils::getRandomString(32);

    int ret = 0;
    int nVersion = 0;

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
