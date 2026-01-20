#ifndef NETWORKUTILITIES_H
#define NETWORKUTILITIES_H

#include <QRegularExpression>
#include <QRegExp>
#include <QString>
#include <QHostAddress>
#include <QNetworkReply>
#include <QtNetwork/qnetworkinterface.h>

class NetworkUtilities : public QObject
{
    Q_OBJECT
public:
    static QString getIPAddress(const QString &host);
    static QString getStringBetween(const QString &s, const QString &a, const QString &b);
    static bool checkIPv4Format(const QString &ip);
    static bool checkIpSubnetFormat(const QString &ip);
    static bool checkIpv6Enabled();
    static QPair<QString, QNetworkInterface> getGatewayAndIface();
    // Returns the Interface Index that could Route to dst
    static int AdapterIndexTo(const QHostAddress& dst);

    static QRegularExpression ipAddressRegExp();
    static QRegularExpression ipAddressPortRegExp();
    static QRegExp ipAddressWithSubnetRegExp();
    static QRegExp ipNetwork24RegExp();
    static QRegExp ipPortRegExp();
    static QRegExp domainRegExp();

    static QString netMaskFromIpWithSubnet(const QString ip);
    static QString ipAddressFromIpWithSubnet(const QString ip);
    static QStringList summarizeRoutes(const QStringList &ips, const QString cidr);

    // DNS resolution methods
    enum class DnsTransport { Udp, Tcp, Tls, Https, Quic };
    
    static QString resolveDns(const QString &hostname, const QString &dnsServer, DnsTransport transport, 
                              quint16 port, int timeoutMsecs = 3000, const QString &dohEndpoint = "/dns-query");
    static QString resolveDnsOverUdp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    static QString resolveDnsOverTcp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    static QString resolveDnsOverTls(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    static QString resolveDnsOverHttps(const QString &hostname, const QString &dnsServer, const QString &endpoint, int timeoutMsecs = 3000);
    static QString resolveDnsOverQuic(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    
    // DNS tunneling - send/receive data via DNS TXT records
    static QByteArray sendViaDnsTunnel(const QByteArray &payload, const QString &endpoint, const QString &baseDomain,
                                        const QString &dnsServer, DnsTransport transport, quint16 port, 
                                        int timeoutMsecs = 30000, const QString &dohEndpoint = "/dns-query");
    static QByteArray sendViaDnsTunnelUdp(const QByteArray &payload, const QString &queryName, 
                                           const QString &dnsServer, quint16 port, int timeoutMsecs);
    static QByteArray sendViaDnsTunnelTcp(const QByteArray &payload, const QString &queryName,
                                           const QString &dnsServer, quint16 port, int timeoutMsecs);
    static QByteArray sendViaDnsTunnelTls(const QByteArray &payload, const QString &queryName,
                                           const QString &dnsServer, quint16 port, int timeoutMsecs);
    static QByteArray sendViaDnsTunnelHttps(const QByteArray &payload, const QString &queryName,
                                             const QString &dnsServer, quint16 port, const QString &endpoint, int timeoutMsecs);
    
    // Chunked UDP - for large responses
    static QByteArray sendViaDnsTunnelUdpChunked(const QByteArray &payload, const QString &queryName,
                                                  const QString &dnsServer, quint16 port, int timeoutMsecs);
};

#endif // NETWORKUTILITIES_H
