#include "openvpn_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QJsonObject>

#include "core/server_defs.h"
#include "containers/containers_defs.h"
#include "core/scripts_registry.h"
#include "utils.h"

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const ServerCredentials &credentials,
    DockerContainer container, ErrorCode *errorCode)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = credentials.hostName;

    if (connData.privKey.isEmpty() || connData.request.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::EasyRsaExecutableMissing;
        return connData;
    }

    QString reqFileName = QString("%1/%2.req").
            arg(amnezia::protocols::openvpn::clientsDirPath).
            arg(connData.clientId);

    ErrorCode e = ServerController::uploadTextFileToContainer(container, credentials, connData.request, reqFileName);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    e = signCert(container, credentials, connData.clientId);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.caCert = ServerController::getTextFileFromContainer(container, credentials, amnezia::protocols::openvpn::caCertPath, &e);
    connData.clientCert = ServerController::getTextFileFromContainer(container, credentials,
        QString("%1/%2.crt").arg(amnezia::protocols::openvpn::clientCertPath).arg(connData.clientId), &e);

    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.taKey = ServerController::getTextFileFromContainer(container, credentials, amnezia::protocols::openvpn::taKeyPath, &e);

    if (connData.caCert.isEmpty() || connData.clientCert.isEmpty() || connData.taKey.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::RemoteProcessCrashError;
    }

    return connData;
}

Settings &OpenVpnConfigurator::m_settings()
{
    static Settings s;
    return s;
}

QString OpenVpnConfigurator::genOpenVpnConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QString config = ServerController::replaceVars(amnezia::scriptData(ProtocolScriptType::openvpn_template, container),
            ServerController::genVarsForScript(credentials, container, containerConfig));

    ConnectionData connData = prepareOpenVpnConfig(credentials, container, errorCode);
    if (errorCode && *errorCode) {
        return "";
    }

    config.replace("$OPENVPN_CA_CERT", connData.caCert);
    config.replace("$OPENVPN_CLIENT_CERT", connData.clientCert);
    config.replace("$OPENVPN_PRIV_KEY", connData.privKey);

    if (config.contains("$OPENVPN_TA_KEY")) {
        config.replace("$OPENVPN_TA_KEY", connData.taKey);
    }
    else {
        config.replace("<tls-auth>", "");
        config.replace("</tls-auth>", "");
    }

#if defined Q_OS_MAC || defined(Q_OS_LINUX)
    config.replace("block-outside-dns", "");
#endif

    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    return QJsonDocument(jConfig).toJson();
}

QString OpenVpnConfigurator::processConfigWithLocalSettings(QString jsonConfig)
{
    QJsonObject json = QJsonDocument::fromJson(jsonConfig.toUtf8()).object();
    QString config = json[config_key::config].toString();

    if (m_settings().routeMode() != Settings::VpnAllSites) {
        config.replace("redirect-gateway def1 bypass-dhcp", "");
    }
    else {
        if(!config.contains("redirect-gateway def1 bypass-dhcp")) {
            config.append("redirect-gateway def1 bypass-dhcp\n");
        }
    }

#if (defined Q_OS_MAC || defined(Q_OS_LINUX)) && !defined(Q_OS_ANDROID)
    config.replace("block-outside-dns", "");
    QString dnsConf = QString(
                "\nscript-security 2\n"
                "up %1/update-resolv-conf.sh\n"
                "down %1/update-resolv-conf.sh\n").
            arg(qApp->applicationDirPath());

    config.append(dnsConf);
#endif

    json[config_key::config] = config;
    return QJsonDocument(json).toJson();
}

QString OpenVpnConfigurator::processConfigWithExportSettings(QString jsonConfig)
{
    QJsonObject json = QJsonDocument::fromJson(jsonConfig.toUtf8()).object();
    QString config = json[config_key::config].toString();

    if(!config.contains("redirect-gateway def1 bypass-dhcp")) {
        config.append("redirect-gateway def1 bypass-dhcp\n");
    }

#if (defined Q_OS_MAC || defined(Q_OS_LINUX)) && !defined(Q_OS_ANDROID)
    config.replace("block-outside-dns", "");
#endif

    json[config_key::config] = config;
    return QJsonDocument(json).toJson();
}

ErrorCode OpenVpnConfigurator::signCert(DockerContainer container,
    const ServerCredentials &credentials, QString clientId)
{
    QString script_import = QString("sudo docker exec -i %1 bash -c \"cd /opt/amnezia/openvpn && "
                             "easyrsa import-req %2/%3.req %3\"")
            .arg(ContainerProps::containerToString(container))
            .arg(amnezia::protocols::openvpn::clientsDirPath)
            .arg(clientId);

    QString script_sign = QString("sudo docker exec -i %1 bash -c \"export EASYRSA_BATCH=1; cd /opt/amnezia/openvpn && "
                                    "easyrsa sign-req client %2\"")
            .arg(ContainerProps::containerToString(container))
            .arg(clientId);

    QStringList scriptList {script_import, script_sign};
    QString script = ServerController::replaceVars(scriptList.join("\n"), ServerController::genVarsForScript(credentials, container));

    return ServerController::runScript(credentials, script);
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::createCertRequest()
{
    ConnectionData connData;
    connData.clientId = Utils::getRandomString(32);

    int             ret = 0;
    int             nVersion = 1;

    QByteArray clientIdUtf8 = connData.clientId.toUtf8();

    EVP_PKEY * pKey = EVP_PKEY_new();
    q_check_ptr(pKey);
    RSA * rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
    q_check_ptr(rsa);
    EVP_PKEY_assign_RSA(pKey, rsa);


    // 2. set version of x509 req
    X509_REQ *x509_req = X509_REQ_new();
    ret = X509_REQ_set_version(x509_req, nVersion);
    if (ret != 1) {
        qWarning() << "Could not get X509!";
        goto free_all;
    }

    // 3. set subject of x509 req
    X509_NAME *x509_name = X509_REQ_get_subject_name(x509_req);

    X509_NAME_add_entry_by_txt(x509_name, "C",  MBSTRING_ASC,
                               (unsigned char *)"ORG", -1, -1, 0);
    X509_NAME_add_entry_by_txt(x509_name, "O",  MBSTRING_ASC,
                               (unsigned char *)"", -1, -1, 0);
    X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC,
                               reinterpret_cast<unsigned char const *>(clientIdUtf8.data()), clientIdUtf8.size(), -1, 0);

    // 4. set public key of x509 req
    ret = X509_REQ_set_pubkey(x509_req, pKey);
    if (ret != 1){
        qWarning() << "Could not set pubkey!";
        goto free_all;
    }

    // 5. set sign key of x509 req
    ret = X509_REQ_sign(x509_req, pKey, EVP_sha256());    // return x509_req->signature->length
    if (ret <= 0){
        qWarning() << "Could not sign request!";
        goto free_all;
    }

    // save private key
    BIO * bp_private = BIO_new(BIO_s_mem());
    q_check_ptr(bp_private);
    if (PEM_write_bio_PrivateKey(bp_private, pKey, nullptr, nullptr, 0, nullptr, nullptr) != 1)
    {
        EVP_PKEY_free(pKey);
        BIO_free_all(bp_private);
        qFatal("PEM_write_bio_PrivateKey");
    }

    const char * buffer = nullptr;
    size_t size = BIO_get_mem_data(bp_private, &buffer);
    q_check_ptr(buffer);
    connData.privKey = QByteArray(buffer, size);
    if (connData.privKey.isEmpty()) {
        qFatal("Failed to generate a random private key");
    }
    BIO_free_all(bp_private);

    // save req
    BIO * bio_req = BIO_new(BIO_s_mem());
    PEM_write_bio_X509_REQ(bio_req, x509_req);

    BUF_MEM *bio_buf;
    BIO_get_mem_ptr(bio_req, &bio_buf);
    connData.request = QByteArray(bio_buf->data, bio_buf->length);
    BIO_free(bio_req);


    EVP_PKEY_free(pKey); // this will also free the rsa key

    return connData;

free_all:
    X509_REQ_free(x509_req);
    EVP_PKEY_free(pKey);

    return connData;
}
