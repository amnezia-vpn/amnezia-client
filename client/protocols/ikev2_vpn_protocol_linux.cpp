#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

#include <QThread>

#include <chrono>

#include "core/networkUtilities.h"

#include "logger.h"
#include "ikev2_vpn_protocol_linux.h"
#include "utilities.h"
#include "core/ipcclient.h"
#include <openssl/pkcs12.h>
#include <openssl/bio.h>
#include <openssl/pem.h>


static Ikev2Protocol* self = nullptr;


Ikev2Protocol::Ikev2Protocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    self = this;
    readIkev2Configuration(configuration);
    m_routeGateway = NetworkUtilities::getGatewayAndIface();
    m_vpnGateway = "192.168.43.10";
    m_vpnLocalAddress = "192.168.43.10";
    m_remoteAddress = configuration.value(amnezia::config_key::hostName).toString();
    m_routeMode = configuration.value(amnezia::config_key::splitTunnelType).toInt();
}

Ikev2Protocol::~Ikev2Protocol()
{
    qDebug() << "IpsecProtocol::~IpsecProtocol()";
    Ikev2Protocol::stop();
}

void Ikev2Protocol::stop()
{
    setConnectionState(Vpn::ConnectionState::Disconnected);
    Ikev2Protocol::disconnect_vpn();
    qDebug() << "IpsecProtocol::stop()";
}

void Ikev2Protocol::readIkev2Configuration(const QJsonObject &configuration)
{
    QJsonObject ikev2_data = configuration.value(ProtocolProps::key_proto_config_data(Proto::Ikev2)).toObject();
    m_config = QJsonDocument::fromJson(ikev2_data.value(config_key::config).toString().toUtf8()).object();
}

ErrorCode Ikev2Protocol::start()
{
    STACK_OF(X509) *certstack = sk_X509_new_null();
    BIO *p12 = BIO_new(BIO_s_mem());

    EVP_PKEY *pkey;
    X509 *cert;

    BIO_write(p12, QByteArray::fromBase64(m_config[config_key::cert].toString().toUtf8()),
              QByteArray::fromBase64(m_config[config_key::cert].toString().toUtf8()).size());

    PKCS12 *pkcs12 = d2i_PKCS12_bio(p12, NULL);
    PKCS12_parse(pkcs12, m_config[config_key::password].toString().toStdString().c_str(), &pkey, &cert, &certstack);
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(bio, cert);

    BUF_MEM *mem = NULL;
    BIO_get_mem_ptr(bio, &mem);

    std::string pem(mem->data, mem->length);
    QString alias(pem.c_str());

    IpcClient::Interface()->writeIPsecUserCert(alias, m_config[config_key::userName].toString());
    IpcClient::Interface()->writeIPsecConfig(m_config[config_key::config].toString());
    IpcClient::Interface()->writeIPsecCaCert(m_config[config_key::cacert].toString(), m_config[config_key::userName].toString());
    IpcClient::Interface()->writeIPsecPrivate(m_config[config_key::cert].toString(), m_config[config_key::userName].toString());
    IpcClient::Interface()->writeIPsecPrivatePass(m_config[config_key::password].toString(), m_config[config_key::hostName].toString(),
                                                  m_config[config_key::userName].toString());

    connect_to_vpn("ikev2-vpn");

    if (!IpcClient::Interface()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    QString connectionStatus;

    auto futureResult = IpcClient::Interface()->getTunnelStatus("ikev2-vpn");
    futureResult.waitForFinished();

    if (futureResult.returnValue().isEmpty()) {
        auto futureResult = IpcClient::Interface()->getTunnelStatus("ikev2-vpn");
        futureResult.waitForFinished();
    }

    connectionStatus = futureResult.returnValue();

    if (connectionStatus.contains("ESTABLISHED")) {
        QStringList lines = connectionStatus.split('\n');
        for (auto iter = lines.begin(); iter!=lines.end(); iter++)
        {
            if (iter->contains("0.0.0.0/0")) {

                m_routeGateway = iter->split("===", Qt::SkipEmptyParts).first();
                m_routeGateway = m_routeGateway.split("   ").at(2);
                m_routeGateway = m_routeGateway.split("/").first();
                m_vpnLocalAddress = m_routeGateway;
                qDebug() << "m_routeGateway " << m_routeGateway;


                // killSwitch toggle
                if (QVariant(m_config.value(config_key::killSwitchOption).toString()).toBool()) {
                    IpcClient::Interface()->enableKillSwitch(m_config, 0);
                }

                if (m_routeMode == 0) {
                    IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "0.0.0.0/1");
                    IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "128.0.0.0/1");
                    IpcClient::Interface()->routeAddList(m_routeGateway, QStringList() << m_remoteAddress);
                }

                IpcClient::Interface()->StopRoutingIpv6();

            }
        }
        setConnectionState(Vpn::ConnectionState::Connected);
    } else {
        setConnectionState(Vpn::ConnectionState::Disconnected);
    }

    return ErrorCode::NoError;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::create_new_vpn(const QString & vpn_name,
                                   const QString & serv_addr) {
   qDebug() << "Ikev2Protocol::create_new_vpn()";
   return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::delete_vpn_connection(const QString &vpn_name) {

    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::connect_to_vpn(const QString &vpn_name) {
    IpcClient::Interface()->startIPsec(vpn_name);
    QThread::msleep(3000);
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Ikev2Protocol::disconnect_vpn() {
    IpcClient::Interface()->stopIPsec("ikev2-vpn");
    IpcClient::Interface()->disableKillSwitch();
    IpcClient::Interface()->StartRoutingIpv6();
    return true;
}
