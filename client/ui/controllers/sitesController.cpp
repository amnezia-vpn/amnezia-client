#include "sitesController.h"

#include <QCoreApplication>
#include <QFile>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QtConcurrent>

#include "core/utils/constants/configKeys.h"
#include "core/utils/networkUtilities.h"
#include "ui/controllers/systemController.h"

using namespace amnezia;

namespace
{
QJsonObject sitesToJsonObject(const QVariantMap &sites)
{
    return QJsonObject::fromVariantMap(sites);
}

QJsonObject managedRoutingRulesPayload(SecureServersRepository *serversRepository, int serverIndex)
{
    const QVariantMap exceptSites = serversRepository->managedVpnSites(serverIndex, RouteMode::VpnAllExceptSites);
    QJsonObject rules;
    const QJsonObject sites = sitesToJsonObject(exceptSites);
    rules.insert(QStringLiteral("version"), 1);
    rules.insert(configKey::serverExcept, sites);
    rules.insert(configKey::managedSplitTunnelExceptSourceSites, sites);
    rules.insert(configKey::managedSplitTunnelExceptSites, sites);
    if (serversRepository->isManagedSplitTunnelingForceEnabled(serverIndex)) {
        rules.insert(configKey::managedSplitTunnelForceEnabled, true);
    }
    return rules;
}

bool readSitesJson(const QString &fileName, QJsonArray &jsonArray, QString &errorMessage)
{
    QByteArray jsonData;
    if (!SystemController::readFile(fileName, jsonData)) {
        errorMessage = QCoreApplication::translate("SitesController", "Can't open file: %1").arg(fileName);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errorMessage = QCoreApplication::translate("SitesController", "Failed to parse JSON data: %1")
                               .arg(parseError.errorString());
        return false;
    }
    if (!jsonDocument.isArray()) {
        errorMessage = QCoreApplication::translate("SitesController", "The JSON data is not an array");
        return false;
    }

    jsonArray = jsonDocument.array();
    return true;
}

QString sanitizedIpList(const QString &value)
{
    QStringList ips;
    const QStringList tokens = value.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        const QString ip = token.trimmed();
        if (NetworkUtilities::checkIpSubnetFormat(ip) && !ips.contains(ip)) {
            ips.append(ip);
        }
    }
    return ips.join(QStringLiteral(", "));
}
}

SitesController::SitesController(SecureServersRepository *serversRepository,
                                 ServersUiController *serversUiController,
                                 InstallController *installController,
                                 IpSplitTunnelingModel *managedExceptSitesModel,
                                 QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_serversUiController(serversUiController),
      m_installController(installController),
      m_managedExceptSitesModel(managedExceptSitesModel)
{
    reloadManagedSites();
}

int SitesController::currentServerIndex() const
{
    return m_serversUiController ? m_serversUiController->getProcessedServerIndex() : -1;
}

RouteMode SitesController::normalizeRouteMode(int routeMode) const
{
    Q_UNUSED(routeMode)
    return RouteMode::VpnAllExceptSites;
}

QVector<QPair<QString, QString>> SitesController::currentManagedSites(RouteMode routeMode) const
{
    return managedSitesForServer(currentServerIndex(), routeMode);
}

QVector<QPair<QString, QString>> SitesController::managedSitesForServer(int serverIndex, RouteMode routeMode) const
{
    QVector<QPair<QString, QString>> sites;
    const QVariantMap sitesMap = m_serversRepository->managedVpnSites(serverIndex, routeMode);
    for (auto it = sitesMap.constBegin(); it != sitesMap.constEnd(); ++it) {
        sites.append(qMakePair(it.key(), it.value().toString()));
    }
    return sites;
}

QString SitesController::normalizeHostname(const QString &hostname) const
{
    QString normalized = hostname.trimmed();
    if (NetworkUtilities::checkIpSubnetFormat(normalized)) {
        return normalized;
    }

    const QUrl url = QUrl::fromUserInput(normalized);
    if (url.isValid() && !url.host().isEmpty()) {
        normalized = url.host();
    } else {
        normalized.replace(QStringLiteral("https://"), QString(), Qt::CaseInsensitive);
        normalized.replace(QStringLiteral("http://"), QString(), Qt::CaseInsensitive);
        normalized.replace(QStringLiteral("ftp://"), QString(), Qt::CaseInsensitive);
        normalized = normalized.split('/', Qt::SkipEmptyParts).first();
    }
    return normalized.trimmed().toLower();
}

bool SitesController::validateHostname(const QString &hostname) const
{
    if (hostname.isEmpty()) {
        return false;
    }
    return NetworkUtilities::checkIpSubnetFormat(hostname) || NetworkUtilities::domainRegExp().exactMatch(hostname);
}

bool SitesController::canEditManagedSites() const
{
    return m_serversUiController && m_serversUiController->isProcessedServerHasWriteAccess();
}

bool SitesController::isManagedSplitTunnelingForceEnabled() const
{
    return m_serversRepository->isManagedSplitTunnelingForceEnabled(currentServerIndex());
}

bool SitesController::isDefaultManagedSplitTunnelingForceEnabled() const
{
    return m_serversRepository->isManagedSplitTunnelingForceEnabled(m_serversRepository->defaultServerIndex());
}

void SitesController::setManagedSplitTunnelingForceEnabled(bool enabled)
{
    const int serverIndex = currentServerIndex();
    if (!canEditManagedSites() || serverIndex < 0) {
        emit errorOccurred(tr("Server routing rules are available only for server admins"));
        return;
    }

    m_serversRepository->setManagedSplitTunnelingForceEnabled(serverIndex, enabled);
    emit managedSplitTunnelingForceChanged();
    publishManagedSplitTunnelingRules(serverIndex);
}

void SitesController::addManagedSite(int routeMode, const QString &hostname)
{
    const int serverIndex = currentServerIndex();
    const RouteMode mode = normalizeRouteMode(routeMode);
    const QString normalizedHostname = normalizeHostname(hostname);
    if (!canEditManagedSites() || serverIndex < 0) {
        emit errorOccurred(tr("Server routing rules are available only for server admins"));
        return;
    }
    if (!validateHostname(normalizedHostname)) {
        emit errorOccurred(tr("Site should be a domain, IP address, or subnet"));
        return;
    }

    m_serversRepository->addManagedVpnSite(serverIndex, mode, normalizedHostname, QString());
    reloadManagedSites();
    publishManagedSplitTunnelingRules(serverIndex);
    emit finished(tr("Managed site updated: %1").arg(normalizedHostname));
}

void SitesController::removeManagedSite(int routeMode, int index)
{
    const int serverIndex = currentServerIndex();
    const RouteMode mode = normalizeRouteMode(routeMode);
    const QVector<QPair<QString, QString>> sites = currentManagedSites(mode);
    if (!canEditManagedSites() || serverIndex < 0 || index < 0 || index >= sites.size()) {
        return;
    }

    const QString hostname = sites.at(index).first;
    m_serversRepository->removeManagedVpnSite(serverIndex, mode, hostname);
    reloadManagedSites();
    publishManagedSplitTunnelingRules(serverIndex);
    emit finished(tr("Managed site removed: %1").arg(hostname));
}

void SitesController::removeManagedSites(int routeMode)
{
    const int serverIndex = currentServerIndex();
    if (!canEditManagedSites() || serverIndex < 0) {
        return;
    }

    m_serversRepository->removeAllManagedVpnSites(serverIndex, normalizeRouteMode(routeMode));
    reloadManagedSites();
    publishManagedSplitTunnelingRules(serverIndex);
    emit finished(tr("Site list cleared!"));
}

void SitesController::importManagedSites(int routeMode, const QString &fileName, bool replaceExisting)
{
    const int serverIndex = currentServerIndex();
    const RouteMode mode = normalizeRouteMode(routeMode);
    if (!canEditManagedSites() || serverIndex < 0) {
        emit errorOccurred(tr("Server routing rules are available only for server admins"));
        return;
    }

    QJsonArray jsonArray;
    QString errorMessage;
    if (!readSitesJson(fileName, jsonArray, errorMessage)) {
        emit errorOccurred(errorMessage);
        return;
    }

    QMap<QString, QString> sites;
    for (const auto &jsonValue : jsonArray) {
        const QJsonObject jsonObject = jsonValue.toObject();
        const QString hostname = normalizeHostname(jsonObject.value("hostname").toString(jsonObject.value("url").toString()));
        if (!validateHostname(hostname)) {
            qDebug() << hostname << "not look like ip address, subnet, or domain name";
            continue;
        }
        const QString fallbackIps = sanitizedIpList(jsonObject.value("ip").toString());
        sites.insert(hostname, NetworkUtilities::checkIpSubnetFormat(hostname) ? QString() : fallbackIps);
    }

    if (replaceExisting) {
        m_serversRepository->removeAllManagedVpnSites(serverIndex, mode);
    }
    m_serversRepository->addManagedVpnSites(serverIndex, mode, sites);
    reloadManagedSites();
    publishManagedSplitTunnelingRules(serverIndex);
    emit finished(tr("Import completed"));
}

void SitesController::exportManagedSites(int routeMode, const QString &fileName)
{
    const QVector<QPair<QString, QString>> sites = currentManagedSites(normalizeRouteMode(routeMode));
    QJsonArray jsonArray;
    for (const auto &site : sites) {
        QJsonObject jsonObject;
        jsonObject["hostname"] = site.first;
        jsonObject["ip"] = site.second;
        jsonArray.append(jsonObject);
    }

    SystemController::saveFile(fileName, QString::fromUtf8(QJsonDocument(jsonArray).toJson()));
    emit finished(tr("Export completed"));
}

void SitesController::reloadManagedSites()
{
    if (!m_managedExceptSitesModel) {
        return;
    }
    m_managedExceptSitesModel->updateModel(currentManagedSites(RouteMode::VpnAllExceptSites));
}

void SitesController::reloadDefaultManagedSites()
{
    if (!m_managedExceptSitesModel) {
        return;
    }
    m_managedExceptSitesModel->updateModel(
            managedSitesForServer(m_serversRepository->defaultServerIndex(), RouteMode::VpnAllExceptSites));
}

QJsonObject SitesController::managedRoutingRulesPayload(int serverIndex) const
{
    return ::managedRoutingRulesPayload(m_serversRepository, serverIndex);
}

void SitesController::publishManagedSplitTunnelingRules(int serverIndex)
{
    if (serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        return;
    }

    const ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    if (credentials.userName.isEmpty() || credentials.secretData.isEmpty()) {
        return;
    }

    ManagedSplitTunnelingPublishJob job;
    job.serverIndex = serverIndex;
    job.credentials = credentials;
    job.rules = managedRoutingRulesPayload(serverIndex);
    job.container = m_serversRepository->server(serverIndex).defaultContainer();

    for (int i = m_pendingManagedSplitTunnelingPublishJobs.size() - 1; i >= 0; --i) {
        if (m_pendingManagedSplitTunnelingPublishJobs.at(i).serverIndex == serverIndex) {
            m_pendingManagedSplitTunnelingPublishJobs.removeAt(i);
        }
    }
    m_pendingManagedSplitTunnelingPublishJobs.append(job);
    startNextManagedSplitTunnelingPublish();
}

void SitesController::startNextManagedSplitTunnelingPublish()
{
    if (m_isManagedSplitTunnelingPublishInProgress) {
        return;
    }

    while (!m_pendingManagedSplitTunnelingPublishJobs.isEmpty()) {
        const ManagedSplitTunnelingPublishJob job = m_pendingManagedSplitTunnelingPublishJobs.takeFirst();
        if (job.serverIndex < 0 || job.credentials.userName.isEmpty() || job.credentials.secretData.isEmpty()) {
            continue;
        }

        m_isManagedSplitTunnelingPublishInProgress = true;
        auto *watcher = new QFutureWatcher<ErrorCode>(this);
        connect(watcher, &QFutureWatcher<ErrorCode>::finished, this, [this, watcher, serverIndex = job.serverIndex]() {
            const ErrorCode errorCode = watcher->result();
            watcher->deleteLater();
            m_isManagedSplitTunnelingPublishInProgress = false;
            if (errorCode != ErrorCode::NoError) {
                emit errorOccurred(tr("Failed to publish server routing rules for clients"));
                startNextManagedSplitTunnelingPublish();
                return;
            }

            qDebug() << "SitesController: published server routing rules for server" << serverIndex;
            emit managedSplitTunnelingRulesPublished(serverIndex);
            startNextManagedSplitTunnelingPublish();
        });

        watcher->setFuture(QtConcurrent::run([installController = m_installController, job]() {
            return installController->publishServerRoutingRules(job.credentials, job.rules, job.container);
        }));
        return;
    }
}
