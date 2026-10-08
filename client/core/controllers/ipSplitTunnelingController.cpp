#include "ipSplitTunnelingController.h"
#include "core/utils/networkUtilities.h"
#include <QDebug>
#include <QJsonObject>
#include <QUrl>

#include <algorithm>

IpSplitTunnelingController::IpSplitTunnelingController(SecureAppSettingsRepository* appSettingsRepository, QObject* parent)
    : QObject(parent),
      m_appSettingsRepository(appSettingsRepository)
{
    m_currentRouteMode = m_appSettingsRepository->routeMode();
    if (m_currentRouteMode == RouteMode::VpnAllSites) { // for old split tunneling configs
        m_appSettingsRepository->setRouteMode(RouteMode::VpnOnlyForwardSites);
        m_currentRouteMode = RouteMode::VpnOnlyForwardSites;
    }
    fillSites();
}

bool IpSplitTunnelingController::addSiteInternal(const QString &hostname, const QStringList &ips)
{
    QVariantMap existing = m_appSettingsRepository->vpnSites(m_currentRouteMode);
    if (existing.contains(hostname) && ips.isEmpty()) {
        return false;
    }

    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname) {
            bool changed = false;
            for (const QString &ip : ips) {
                if (!ip.isEmpty() && !m_sites[i].second.contains(ip)) {
                    m_sites[i].second.append(ip);
                    changed = true;
                }
            }
            if (!changed) {
                return false;
            }
            m_appSettingsRepository->addVpnSite(m_currentRouteMode, hostname, ips);
            return true;
        }
    }
    m_sites.append(qMakePair(hostname, ips));
    m_appSettingsRepository->addVpnSite(m_currentRouteMode, hostname, ips);
    return true;
}

void IpSplitTunnelingController::addSites(const QMap<QString, QStringList> &sites, bool replaceExisting)
{
    if (replaceExisting) {
        m_sites.clear();
    }
    for (auto it = sites.constBegin(); it != sites.constEnd(); ++it) {
        const QString &hostname = it.key();
        const QStringList &ips = it.value();
        bool found = false;
        for (int i = 0; i < m_sites.size(); i++) {
            if (m_sites[i].first == hostname) {
                for (const QString &ip : ips) {
                    if (!ip.isEmpty() && !m_sites[i].second.contains(ip)) {
                        m_sites[i].second.append(ip);
                    }
                }
                found = true;
                break;
            }
        }
        if (!found) {
            m_sites.append(qMakePair(hostname, ips));
        }
    }
    if (replaceExisting) {
        m_appSettingsRepository->removeAllVpnSites(m_currentRouteMode);
    }
    m_appSettingsRepository->addVpnSites(m_currentRouteMode, sites);
}

bool IpSplitTunnelingController::addSite(const QString &hostname)
{
    QString normalizedHostname = normalizeHostname(hostname);
    
    if (!validateHostname(normalizedHostname)) {
        return false;
    }
    
    if (validateIpv4Cidr(normalizedHostname)) {
        processSite(normalizedHostname, {});
        return true;
    }
    
    if (addSiteInternal(normalizedHostname, {})) {
        const RouteMode routeMode = m_currentRouteMode;
        QHostInfo::lookupHost(normalizedHostname, this,
                              [this, routeMode, normalizedHostname](const QHostInfo &hostInfo) {
            QStringList allIpv4;
            for (const QHostAddress &address : hostInfo.addresses()) {
                if (address.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                    allIpv4.append(address.toString());
                }
            }
            allIpv4.removeDuplicates();
            allIpv4.sort();

            if (!allIpv4.isEmpty()) {
                processSiteAfterResolve(routeMode, normalizedHostname, allIpv4);
            }
        });
        return true;
    }
    
    return false;
}

bool IpSplitTunnelingController::removeSite(const QString &hostname)
{
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname) {
            m_sites.removeAt(i);
            m_appSettingsRepository->removeVpnSite(m_currentRouteMode, hostname);
            return true;
        }
    }
    return false;
}

void IpSplitTunnelingController::removeSites()
{
    m_sites.clear();
    m_appSettingsRepository->removeAllVpnSites(m_currentRouteMode);
}

void IpSplitTunnelingController::setRouteMode(RouteMode routeMode)
{
    m_currentRouteMode = routeMode;
    fillSites();
    m_appSettingsRepository->setRouteMode(routeMode);
}

void IpSplitTunnelingController::toggleSplitTunneling(bool enabled)
{
    m_appSettingsRepository->setSitesSplitTunnelingEnabled(enabled);
}

RouteMode IpSplitTunnelingController::getRouteMode() const
{
    return m_currentRouteMode;
}

bool IpSplitTunnelingController::isSplitTunnelingEnabled() const
{
    return m_appSettingsRepository->isSitesSplitTunnelingEnabled();
}

QVector<QPair<QString, QStringList>> IpSplitTunnelingController::getCurrentSites() const
{
    return m_sites;
}

void IpSplitTunnelingController::fillSites()
{
    QVariantMap sitesMap = m_appSettingsRepository->vpnSites(m_currentRouteMode);
    m_sites.clear();
    for (auto it = sitesMap.begin(); it != sitesMap.end(); ++it) {
        m_sites.append(qMakePair(it.key(), SecureAppSettingsRepository::siteIpList(it.value())));
    }
}

QString IpSplitTunnelingController::normalizeHostname(const QString &hostname) const
{
    const QString candidate = hostname.trimmed();
    if (candidate.isEmpty()) {
        return {};
    }

    if (validateIpv4Cidr(candidate)) {
        return candidate;
    }

    const qsizetype slashIndex = candidate.indexOf('/');
    if (slashIndex > 0 && NetworkUtilities::checkIPv4Format(candidate.left(slashIndex))) {
        return {};
    }

    const QString urlText = candidate.contains("://") ? candidate : QStringLiteral("https://") + candidate;
    const QUrl url(urlText, QUrl::StrictMode);
    if (!url.isValid() || url.host().isEmpty()) {
        return {};
    }

    QString normalized = QString::fromLatin1(QUrl::toAce(url.host())).toLower();
    while (normalized.endsWith('.')) {
        normalized.chop(1);
    }
    return normalized;
}

bool IpSplitTunnelingController::validateHostname(const QString &hostname) const
{
    if (validateIpv4Cidr(hostname)) {
        return true;
    }

    if (hostname.isEmpty() || hostname.size() > 253 || !hostname.contains('.') || hostname.contains('/')) {
        return false;
    }

    const QStringList labels = hostname.split('.', Qt::KeepEmptyParts);
    if (labels.constLast().size() < 2) {
        return false;
    }
    for (const QString &label : labels) {
        if (label.isEmpty() || label.size() > 63 || !label.front().isLetterOrNumber() || !label.back().isLetterOrNumber()) {
            return false;
        }
        for (const QChar character : label) {
            if (!character.isLetterOrNumber() && character != '-') {
                return false;
            }
        }
    }

    return std::any_of(labels.constLast().cbegin(), labels.constLast().cend(), [](QChar character) {
        return character.isLetter();
    });
}

bool IpSplitTunnelingController::validateIpv4Cidr(const QString &value) const
{
    const QStringList parts = value.split('/', Qt::KeepEmptyParts);
    if (parts.size() == 1) {
        return NetworkUtilities::ipAddressRegExp().match(parts.constFirst()).hasMatch();
    }
    if (parts.size() != 2 || !NetworkUtilities::ipAddressRegExp().match(parts.constFirst()).hasMatch()) {
        return false;
    }

    const QString prefix = parts.constLast();
    if (prefix.isEmpty() || (prefix.size() > 1 && prefix.startsWith('0'))) {
        return false;
    }
    for (const QChar character : prefix) {
        if (character < '0' || character > '9') {
            return false;
        }
    }

    bool converted = false;
    const int prefixLength = prefix.toInt(&converted);
    return converted && prefixLength >= 0 && prefixLength <= 32;
}

void IpSplitTunnelingController::processSiteAfterResolve(RouteMode routeMode, const QString &hostname,
                                                         const QStringList &ips)
{
    const QVariantMap sites = m_appSettingsRepository->vpnSites(routeMode);
    if (!sites.contains(hostname)) {
        return;
    }

    m_appSettingsRepository->addVpnSite(routeMode, hostname, ips);
    if (m_currentRouteMode != routeMode) {
        return;
    }

    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname) {
            for (const QString &ip : ips) {
                if (!ip.isEmpty() && !m_sites[i].second.contains(ip)) {
                    m_sites[i].second.append(ip);
                }
            }
            break;
        }
    }
}

void IpSplitTunnelingController::processSite(const QString &hostname, const QStringList &ips)
{
    addSiteInternal(hostname, ips);
}

bool IpSplitTunnelingController::importSitesFromJson(const QByteArray& jsonData, bool replaceExisting, QString &errorMessage)
{
    QJsonParseError parseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        errorMessage = tr("Failed to parse JSON data: %1").arg(parseError.errorString());
        return false;
    }
    
    if (!jsonDocument.isArray()) {
        errorMessage = tr("The JSON data is not an array");
        return false;
    }
    
    QJsonArray jsonArray = jsonDocument.array();
    QMap<QString, QStringList> sites;
    
    for (auto jsonValue : jsonArray) {
        if (!jsonValue.isObject()) {
            continue;
        }

        const QJsonObject jsonObject = jsonValue.toObject();
        QString hostname = jsonObject.value("hostname").toString("");
        QString normalizedHostname = normalizeHostname(hostname);

        if (!validateHostname(normalizedHostname)) {
            qDebug() << normalizedHostname << " not look like ip adress or domain name";
            continue;
        }

        QStringList ips;
        if (jsonObject.value("ips").isArray()) {
            const QJsonArray ipsArray = jsonObject.value("ips").toArray();
            for (const QJsonValue &ipValue : ipsArray) {
                const QString ip = ipValue.toString().trimmed();
                if (!ip.contains('/') && validateIpv4Cidr(ip)) {
                    ips.append(ip);
                }
            }
        }

        const QString legacyIp = jsonObject.value("ip").toString().trimmed();
        if (!legacyIp.contains('/') && validateIpv4Cidr(legacyIp)) {
            ips.append(legacyIp);
        }
        ips.removeDuplicates();
        ips.sort();

        QStringList mergedIps = sites.value(normalizedHostname);
        for (const QString &ip : ips) {
            if (!mergedIps.contains(ip)) {
                mergedIps.append(ip);
            }
        }
        sites.insert(normalizedHostname, mergedIps);
    }

    if (sites.isEmpty()) {
        errorMessage = tr("The JSON data does not contain valid sites");
        return false;
    }

    addSites(sites, replaceExisting);
    
    return true;
}

QByteArray IpSplitTunnelingController::exportSitesToJson() const
{
    QVector<QPair<QString, QStringList>> sites = getCurrentSites();
    QJsonArray jsonArray;
    
    for (const auto &site : sites) {
        QJsonObject jsonObject;
        jsonObject["hostname"] = site.first;

        QJsonArray ipsArray;
        for (const QString &ip : site.second) {
            ipsArray.append(ip);
        }
        jsonObject["ips"] = ipsArray;
        jsonObject["ip"] = site.second.isEmpty() ? QString() : site.second.first();
        jsonArray.append(jsonObject);
    }
    
    QJsonDocument jsonDocument(jsonArray);
    return jsonDocument.toJson();
}
