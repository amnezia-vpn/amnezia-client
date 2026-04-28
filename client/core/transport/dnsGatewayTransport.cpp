#include "dnsGatewayTransport.h"

#include <QDebug>
#include <QHostAddress>
#include <QHostInfo>
#include <QSharedPointer>
#include <QStringList>

#include "dns/dnsTunnel.h"
#include "core/networkUtilities.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/ipcclient.h"
#endif

namespace amnezia::transport
{

DnsGatewayTransport::DnsGatewayTransport(dns::DnsProtocol protocol,
                                         const QString &dnsServer,
                                         const QString &baseDomain,
                                         quint16 port,
                                         int timeoutMsecs,
                                         bool isStrictKillSwitchEnabled,
                                         const QString &dohEndpoint)
    : m_protocol(protocol),
      m_dnsServer(dnsServer),
      m_baseDomain(baseDomain),
      m_port(port),
      m_timeoutMsecs(timeoutMsecs),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled),
      m_dohEndpoint(dohEndpoint)
{
}

QString DnsGatewayTransport::name() const
{
    switch (m_protocol) {
    case dns::DnsProtocol::Udp:   return QStringLiteral("DNS-UDP");
    case dns::DnsProtocol::Tcp:   return QStringLiteral("DNS-TCP");
    case dns::DnsProtocol::Tls:   return QStringLiteral("DNS-DoT");
    case dns::DnsProtocol::Https: return QStringLiteral("DNS-DoH");
    case dns::DnsProtocol::Quic:  return QStringLiteral("DNS-DoQ");
    }
    return QStringLiteral("DNS");
}

QString DnsGatewayTransport::resolveServerOnce()
{
    if (m_resolved.load()) {
        return m_resolvedServerIp;
    }

    QHostAddress addr(m_dnsServer);
    if (!addr.isNull()) {
        m_resolvedServerIp = m_dnsServer;
    } else {
        QHostInfo info = QHostInfo::fromName(m_dnsServer);
        if (!info.addresses().isEmpty()) {
            m_resolvedServerIp = info.addresses().first().toString();
        } else {
            m_resolvedServerIp = m_dnsServer;
        }
    }
    m_resolved.store(true);
    return m_resolvedServerIp;
}

void DnsGatewayTransport::applyKillSwitchAllowlist(const QString &ip)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_isStrictKillSwitchEnabled || ip.isEmpty()) {
        return;
    }
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(QStringList { ip });
        if (!reply.waitForFinished(1000) || !reply.returnValue()) {
            qWarning() << "DnsGatewayTransport: addKillSwitchAllowedRange failed for" << ip;
        }
    });
#else
    Q_UNUSED(ip)
#endif
}

amnezia::ErrorCode DnsGatewayTransport::send(const QString &endpointTemplate,
                                             const QByteArray &requestBody,
                                             QByteArray &decryptedResponse,
                                             const DecryptionHook &decryptionHook)
{
    QString endpointName = endpointTemplate;
    endpointName.remove("%1");
    if (endpointName.startsWith(QLatin1String("v1/"))) {
        endpointName = endpointName.mid(3);
    }
    while (endpointName.endsWith(QLatin1Char('/'))) {
        endpointName.chop(1);
    }
    while (endpointName.startsWith(QLatin1Char('/'))) {
        endpointName = endpointName.mid(1);
    }

    qDebug() << "[DNS-Transport]" << name() << "send() endpointTemplate=" << endpointTemplate
             << "endpointName=" << endpointName << "baseDomain=" << m_baseDomain
             << "server=" << m_dnsServer << "port=" << m_port
             << "dohPath=" << m_dohEndpoint << "timeoutMs=" << m_timeoutMsecs
             << "requestBodyBytes=" << requestBody.size();

    if (endpointName.isEmpty() || m_baseDomain.isEmpty() || m_dnsServer.isEmpty()) {
        qWarning() << "[DNS-Transport] ABORT: empty endpoint/baseDomain/server";
        return amnezia::ErrorCode::AmneziaServiceConnectionFailed;
    }

    const bool needsHostname = (m_protocol == dns::DnsProtocol::Tls
                                || m_protocol == dns::DnsProtocol::Https);

    QString serverIp = resolveServerOnce();
    QString serverForRequest = needsHostname ? m_dnsServer : serverIp;

    qDebug() << "[DNS-Transport] resolved server IP=" << serverIp
             << "serverForRequest=" << serverForRequest
             << "needsHostname=" << needsHostname;

    applyKillSwitchAllowlist(serverIp);

    const QByteArray encrypted = dns::DnsTunnel::send(requestBody,
                                                      endpointName,
                                                      m_baseDomain,
                                                      serverForRequest,
                                                      m_protocol,
                                                      m_port,
                                                      m_timeoutMsecs,
                                                      m_dohEndpoint);
    qDebug() << "[DNS-Transport] DnsTunnel::send returned" << encrypted.size() << "bytes";
    if (encrypted.isEmpty()) {
        qWarning() << "[DNS-Transport] DnsTunnel returned empty payload, treat as connection failure";
        return amnezia::ErrorCode::AmneziaServiceConnectionFailed;
    }

    if (!decryptionHook) {
        qCritical() << "[DNS-Transport] decryption hook is null";
        return amnezia::ErrorCode::ApiConfigDecryptionError;
    }

    DecryptionResult decrypted = decryptionHook(encrypted);
    if (!decrypted.isOk) {
        qCritical() << "[DNS-Transport] response decryption failed (encrypted bytes="
                    << encrypted.size() << ")";
        return amnezia::ErrorCode::ApiConfigDecryptionError;
    }

    qDebug() << "[DNS-Transport] success, decrypted response bytes=" << decrypted.decrypted.size();
    decryptedResponse = decrypted.decrypted;
    return amnezia::ErrorCode::NoError;
}

} // namespace amnezia::transport
