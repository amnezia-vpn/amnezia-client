#include "networkUtilities.h"

#ifdef Q_OS_WIN
    #include <QSettings>
#endif

#include <QHostAddress>
#include <QHostInfo>

QRegularExpression NetworkUtilities::ipAddressRegExp()
{
    return QRegularExpression("^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\\.(?!$)|$)){4}$");
}

QRegExp NetworkUtilities::ipAddressWithSubnetRegExp()
{
    return QRegExp("(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
                   "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])"
                   "(\\/(?:[0-9]|[1-2][0-9]|3[0-2])){0,1}");
}

QRegExp NetworkUtilities::ipNetwork24RegExp()
{
    return QRegExp("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
                   "0$");
}

QRegExp NetworkUtilities::ipPortRegExp()
{
    return QRegExp("^()([1-9]|[1-5]?[0-9]{2,4}|6[1-4][0-9]{3}|65[1-4][0-9]{2}|655[1-2][0-9]|6553[1-5])$");
}

QRegExp NetworkUtilities::domainRegExp()
{
    return QRegExp("(((?!\\-))(xn\\-\\-)?[a-z0-9\\-_]{0,61}[a-z0-9]{1,1}\\.)*(xn\\-\\-)?([a-z0-9\\-]{1,61}|[a-z0-"
                   "9\\-]{1,30})\\.[a-z]{2,}");
}

QStringList NetworkUtilities::summarizeRoutes(const QStringList &ips, const QString cidr)
{
    //    QMap<int, int>
    //    QHostAddress

           //    QMap<QString, QStringList> subnets; // <"a.b", <list subnets>>

           //    for (const QString &ip : ips) {
           //        if (ip.count(".") != 3) continue;

           //        const QStringList &parts = ip.split(".");
           //        subnets[parts.at(0) + "." + parts.at(1)].append(ip);
           //    }

    return QStringList();
}

QString NetworkUtilities::getIPAddress(const QString &host)
{
    QHostAddress address(host);
    if (QAbstractSocket::IPv4Protocol == address.protocol()) {
        return host;
    } else if (QAbstractSocket::IPv6Protocol == address.protocol()) {
        return host;
    }

    QList<QHostAddress> addresses = QHostInfo::fromName(host).addresses();
    if (!addresses.isEmpty()) {
        return addresses.first().toString();
    }
    qDebug() << "Unable to resolve address for " << host;
    return "";
}

QString NetworkUtilities::getStringBetween(const QString &s, const QString &a, const QString &b)
{
    int ap = s.indexOf(a), bp = s.indexOf(b, ap + a.length());
    if (ap < 0 || bp < 0)
        return QString();
    ap += a.length();
    if (bp - ap <= 0)
        return QString();
    return s.mid(ap, bp - ap).trimmed();
}

bool NetworkUtilities::checkIPv4Format(const QString &ip)
{
    if (ip.isEmpty())
        return false;
    int count = ip.count(".");
    if (count != 3)
        return false;

    QHostAddress addr(ip);
    return (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol);
}

bool NetworkUtilities::checkIpSubnetFormat(const QString &ip)
{
    if (!ip.contains("/"))
        return checkIPv4Format(ip);

    QStringList parts = ip.split("/");
    if (parts.size() != 2)
        return false;

    bool ok;
    int subnet = parts.at(1).toInt(&ok);
    if (subnet >= 0 && subnet <= 32 && ok)
        return checkIPv4Format(parts.at(0));
    else
        return false;
}

bool NetworkUtilities::checkIpv6Enabled() {
#ifdef Q_OS_WIN
    QSettings RegHLM("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters",
                     QSettings::NativeFormat);
    int ret = RegHLM.value("DisabledComponents", 0).toInt();
    qDebug() << "Check for Windows disabled IPv6 return " << ret;
    return (ret != 255);
#endif
    return true;
}
