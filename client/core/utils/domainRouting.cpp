#include "domainRouting.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>

namespace
{
    /// <summary>Проверяет каноническое доменное имя.</summary>
    /// <param name="name">Имя в ASCII-представлении.</param>
    /// <returns>Признак допустимого DNS-имени.</returns>
    bool validDomain(const QString &name)
    {
        if (name.isEmpty() || name.size() > 253 || !name.contains('.')) {
            return false;
        }

        static const QRegularExpression labelPattern(QStringLiteral("^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?$"));
        const QStringList labels = name.split('.');
        for (const QString &label : labels) {
            if (!labelPattern.match(label).hasMatch()) {
                return false;
            }
        }

        return QRegularExpression(QStringLiteral("[a-z]")).match(labels.last()).hasMatch();
    }

    /// <summary>Создаёт доменное условие Xray без совпадения по подстроке.</summary>
    /// <param name="rule">Проверенное доменное правило.</param>
    /// <returns>Условие точного совпадения либо маска поддоменов.</returns>
    QString domainCondition(const amnezia::domainRouting::Rule &rule)
    {
        if (rule.kind == amnezia::domainRouting::RuleKind::Domain) {
            return QStringLiteral("full:") + rule.value;
        }

        QString suffix = rule.value.mid(2);
        suffix.replace('.', QStringLiteral("\\."));
        return QStringLiteral("regexp:^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\\.)+") + suffix + QStringLiteral("$");
    }

    /// <summary>Проверяет явно заданный исходящий путь.</summary>
    /// <param name="path">Интерфейс, адрес источника и DNS-серверы пути.</param>
    /// <returns>Признак пригодности пути для IPv4-маршрутизации.</returns>
    bool validPath(const amnezia::domainRouting::Path &path)
    {
        const QHostAddress source(path.sourceAddress);
        if (path.interfaceName.isEmpty() || source.protocol() != QAbstractSocket::IPv4Protocol || source.isMulticast()
            || source == QHostAddress::AnyIPv4 || path.dnsServers.isEmpty()) {
            return false;
        }

        for (const QString &server : path.dnsServers) {
            const QHostAddress address(server);
            if (address.protocol() != QAbstractSocket::IPv4Protocol || address.isMulticast()
                || address == QHostAddress::AnyIPv4) {
                return false;
            }
        }

        return true;
    }

    /// <summary>Создаёт исходящий обработчик с привязкой к сетевому пути.</summary>
    /// <param name="path">Интерфейс и адрес источника.</param>
    /// <param name="tag">Идентификатор исходящего обработчика.</param>
    /// <returns>Описание исходящего обработчика.</returns>
    QJsonObject outbound(const amnezia::domainRouting::Path &path, const QString &tag)
    {
        return { { "tag", tag },
                 { "protocol", "freedom" },
                 { "sendThrough", path.sourceAddress },
                 { "settings", QJsonObject { { "domainStrategy", "ForceIPv4" } } },
                 { "streamSettings",
                   QJsonObject { { "sockopt", QJsonObject { { "interface", path.interfaceName } } } } } };
    }
}

std::optional<amnezia::domainRouting::Rule> amnezia::domainRouting::parseRule(const QString &text)
{
    const QString candidate = text.trimmed();
    if (candidate.isEmpty()) {
        return std::nullopt;
    }

    const QHostAddress address(candidate);
    if (!address.isNull() && address.scopeId().isEmpty()) {
        return Rule { RuleKind::Network, address.toString() };
    }

    const qsizetype slash = candidate.indexOf('/');
    if (slash > 0 && !QHostAddress(candidate.left(slash)).isNull()) {
        static const QRegularExpression prefixPattern(QStringLiteral("^(0|[1-9][0-9]{0,2})$"));
        if (!prefixPattern.match(candidate.mid(slash + 1)).hasMatch()) {
            return std::nullopt;
        }

        const QPair<QHostAddress, int> subnet = QHostAddress::parseSubnet(candidate);
        if (subnet.second < 0 || !subnet.first.scopeId().isEmpty()) {
            return std::nullopt;
        }

        return Rule { RuleKind::Network, subnet.first.toString() + '/' + QString::number(subnet.second) };
    }

    const bool wildcard = candidate.startsWith(QStringLiteral("*."));
    QString domain;
    if (wildcard) {
        domain = candidate.mid(2);
        if (domain.contains('/') || domain.contains(':') || domain.contains('?') || domain.contains('#')) {
            return std::nullopt;
        }
    } else {
        const QUrl url(candidate.contains(QStringLiteral("://")) ? candidate : QStringLiteral("https://") + candidate,
                       QUrl::StrictMode);
        if (!url.isValid()
            || (url.scheme() != QStringLiteral("https") && url.scheme() != QStringLiteral("http")
                && url.scheme() != QStringLiteral("ftp"))
            || !url.userInfo().isEmpty()) {
            return std::nullopt;
        }

        domain = url.host();
    }

    if (domain.endsWith('.')) {
        domain.chop(1);
    }

    domain = QString::fromLatin1(QUrl::toAce(domain)).toLower();
    const QHostAddress hostAddress(domain);
    if (!wildcard && !hostAddress.isNull() && hostAddress.scopeId().isEmpty()) {
        return Rule { RuleKind::Network, hostAddress.toString() };
    }

    if (!validDomain(domain)) {
        return std::nullopt;
    }

    return Rule { wildcard ? RuleKind::Subdomains : RuleKind::Domain, wildcard ? QStringLiteral("*.") + domain : domain };
}

QJsonObject amnezia::domainRouting::buildConfig(const Options &options, QString &error)
{
    error.clear();
    if (options.mode != RouteMode::VpnOnlyForwardSites && options.mode != RouteMode::VpnAllExceptSites) {
        error = QStringLiteral("Не задан режим раздельной маршрутизации сайтов.");
        return { };
    }

    if (!validPath(options.direct) || !validPath(options.tunnel)) {
        error = QStringLiteral("Не задан интерфейс, IPv4-адрес или DNS-сервер исходящего пути.");
        return { };
    }

    if (options.tunName.isEmpty()
        && (!options.socksPort || !options.dnsPort || options.socksPort == options.dnsPort
            || options.socksUser.isEmpty() || options.socksPassword.isEmpty())) {
        error = QStringLiteral("Некорректные параметры локальных слушателей.");
        return { };
    }

    if (options.appsMode < AppsRouteMode::VpnAllApps || options.appsMode > AppsRouteMode::VpnAllExceptApps
        || options.mtu < 1280) {
        error = QStringLiteral("Некорректный режим приложений или MTU.");
        return { };
    }

    QJsonArray domains;
    QJsonArray networks;
    for (const QString &text : options.rules) {
        const std::optional<Rule> rule = parseRule(text);
        if (!rule) {
            error = QStringLiteral("Список содержит некорректное правило маршрутизации.");
            return { };
        }

        if (rule->kind == RuleKind::Network) {
            if (!networks.contains(rule->value)) {
                networks.append(rule->value);
            }
        } else {
            const QString condition = domainCondition(*rule);
            if (!domains.contains(condition)) {
                domains.append(condition);
            }
        }
    }

    const bool include = options.mode == RouteMode::VpnOnlyForwardSites;
    const QString selectedTag = include ? QStringLiteral("tunnel") : QStringLiteral("direct");
    const QString defaultTag = include ? QStringLiteral("direct") : QStringLiteral("tunnel");
    const Path &selectedPath = include ? options.tunnel : options.direct;
    const Path &defaultPath = include ? options.direct : options.tunnel;

    QJsonArray dnsServers { QJsonObject { { "address", "fakedns" }, { "domains", QJsonArray { "regexp:.*" } } } };
    if (!domains.isEmpty()) {
        for (qsizetype i = 0; i < selectedPath.dnsServers.size(); ++i) {
            dnsServers.append(QJsonObject { { "address", selectedPath.dnsServers.at(i) },
                                            { "domains", domains },
                                            { "skipFallback", true },
                                            { "finalQuery", i == selectedPath.dnsServers.size() - 1 },
                                            { "tag", selectedTag + QStringLiteral("-dns") } });
        }
    }

    for (const QString &server : defaultPath.dnsServers) {
        dnsServers.append(QJsonObject { { "address", server }, { "tag", defaultTag + QStringLiteral("-dns") } });
    }

    QJsonArray routing {
        QJsonObject { { "type", "field" }, { "inboundTag", QJsonArray { "dns-in" } }, { "outboundTag", "dns-out" } },
        QJsonObject { { "type", "field" }, { "inboundTag", QJsonArray { "direct-dns" } }, { "outboundTag", "direct" } },
        QJsonObject { { "type", "field" }, { "inboundTag", QJsonArray { "tunnel-dns" } }, { "outboundTag", "tunnel" } },
        QJsonObject { { "type", "field" }, { "ip", QJsonArray { "::/0" } }, { "outboundTag", "blocked" } }
    };
    if (!options.tunName.isEmpty()) {
        routing.prepend(QJsonObject { { "type", "field" },
                                      { "inboundTag", QJsonArray { "tun-in" } },
                                      { "port", "53" },
                                      { "outboundTag", "dns-out" } });
    }

    const QJsonArray apps = QJsonArray::fromStringList(options.apps);
    if (options.appsMode == AppsRouteMode::VpnOnlyForwardApps && apps.isEmpty()) {
        routing.append(QJsonObject { { "type", "field" }, { "network", "tcp,udp" }, { "outboundTag", "direct" } });
    }
    if (options.appsMode == AppsRouteMode::VpnAllExceptApps && !apps.isEmpty()) {
        routing.append(QJsonObject { { "type", "field" }, { "process", apps }, { "outboundTag", "direct" } });
    }

    QJsonObject siteRule { { "type", "field" }, { "outboundTag", selectedTag } };
    if (options.appsMode == AppsRouteMode::VpnOnlyForwardApps) {
        siteRule.insert("process", apps);
    }

    if (!domains.isEmpty()) {
        QJsonObject rule = siteRule;
        rule.insert("domain", domains);
        routing.append(rule);
    }

    if (!networks.isEmpty()) {
        QJsonObject rule = siteRule;
        rule.insert("ip", networks);
        routing.append(rule);
    }

    QJsonObject fallbackRule { { "type", "field" }, { "network", "tcp,udp" }, { "outboundTag", defaultTag } };
    if (options.appsMode == AppsRouteMode::VpnOnlyForwardApps) {
        fallbackRule.insert("process", apps);
    }

    routing.append(fallbackRule);
    if (options.appsMode == AppsRouteMode::VpnOnlyForwardApps) {
        routing.append(QJsonObject { { "type", "field" }, { "network", "tcp,udp" }, { "outboundTag", "direct" } });
    }

    QJsonObject result {
        { "log", QJsonObject { { "loglevel", "warning" }, { "access", "none" } } },
        { "fakedns", QJsonArray { QJsonObject { { "ipPool", "198.18.0.0/15" }, { "poolSize", 65535 } } } },
        { "dns",
          QJsonObject { { "servers", dnsServers },
                        { "hosts", options.hosts },
                        { "queryStrategy", "UseIPv4" },
                        { "enableParallelQuery", false },
                        { "serveStale", false } } },
        { "inbounds",
          QJsonArray { QJsonObject {
                               { "tag", "socks-in" },
                               { "listen", "127.0.0.1" },
                               { "port", options.socksPort },
                               { "protocol", "socks" },
                               { "settings",
                                 QJsonObject { { "auth", "password" },
                                               { "udp", true },
                                               { "ip", "127.0.0.1" },
                                               { "accounts",
                                                 QJsonArray { QJsonObject { { "user", options.socksUser },
                                                                            { "pass", options.socksPassword } } } } } },
                               { "sniffing",
                                 QJsonObject { { "enabled", true },
                                               { "routeOnly", true },
                                               { "destOverride", QJsonArray { "fakedns", "http", "tls", "quic" } } } } },
                       QJsonObject { { "tag", "dns-in" },
                                     { "listen", "127.0.0.1" },
                                     { "port", options.dnsPort },
                                     { "protocol", "dokodemo-door" },
                                     { "settings",
                                       QJsonObject { { "network", "tcp,udp" },
                                                     { "address", defaultPath.dnsServers.first() },
                                                     { "port", 53 } } } } } },
        { "outbounds",
          QJsonArray {
                  QJsonObject { { "tag", "blocked" }, { "protocol", "blackhole" } },
                  outbound(options.direct, QStringLiteral("direct")), outbound(options.tunnel, QStringLiteral("tunnel")),
                  QJsonObject { { "tag", "dns-out" },
                                { "protocol", "dns" },
                                { "proxySettings", QJsonObject { { "tag", defaultTag } } },
                                { "settings",
                                  QJsonObject { { "address", defaultPath.dnsServers.first() },
                                                { "port", 53 },
                                                { "rules",
                                                  QJsonArray { QJsonObject { { "action", "hijack" }, { "qType", "1,28" } },
                                                               QJsonObject { { "action", "direct" } } } } } } } } },
        { "routing", QJsonObject { { "domainStrategy", "IPOnDemand" }, { "rules", routing } } }
    };

    if (!options.tunName.isEmpty()) {
        const QJsonObject sniffing = result.value("inbounds").toArray().first().toObject().value("sniffing").toObject();
        result.insert("inbounds",
                      QJsonArray { QJsonObject {
                              { "tag", "tun-in" },
                              { "port", 0 },
                              { "protocol", "tun" },
                              { "sniffing", sniffing },
                              { "settings",
                                QJsonObject { { "name", options.tunName },
                                              { "desc", "Amnezia domain routing" },
                                              { "mtu", options.mtu },
                                              { "gateway", QJsonArray { "169.254.254.1/32", "fdfe:dcba:9876::1/128" } },
                                              { "dns", QJsonArray { "169.254.254.2" } },
                                              { "autoOutboundsInterface", "" },
                                              { "autoSystemRoutingTable",
                                                QJsonArray { "0.0.0.0/1", "128.0.0.0/1", "198.18.0.0/15",
                                                             "169.254.254.2/32", "::/1", "8000::/1" } } } } } });
    }

    return result;
}
