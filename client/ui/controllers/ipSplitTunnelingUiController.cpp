#include "ipSplitTunnelingUiController.h"

#include <QDebug>

#include "systemController.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

IpSplitTunnelingUiController::IpSplitTunnelingUiController(IpSplitTunnelingController* ipSplitTunnelingController,
                                                          IpSplitTunnelingModel* ipSplitTunnelingModel, QObject *parent)
    : QObject(parent),
      m_ipSplitTunnelingController(ipSplitTunnelingController),
      m_ipSplitTunnelingModel(ipSplitTunnelingModel)
{
    m_ipSplitTunnelingModel->updateModel(m_ipSplitTunnelingController->getCurrentSites());
}

void IpSplitTunnelingUiController::addSite(QString hostname)
{
    using Result = IpSplitTunnelingController::AddSiteResult;

    QString normalized;
    const Result result = m_ipSplitTunnelingController->addSite(hostname, &normalized);
    const QString shown = normalized.isEmpty() ? hostname.trimmed() : normalized;

    switch (result) {
    case Result::Added:
        emit finished(tr("New site added: %1").arg(shown));
        break;
    case Result::AddedWildcard:
        emit finished(tr("New site added: %1. Wildcard sites apply to WireGuard and "
                         "AmneziaWG connections.")
                              .arg(shown));
        break;
    case Result::Duplicate:
        emit errorOccurred(tr("%1 is already in the list").arg(shown));
        break;
    case Result::Invalid:
        break;
    case Result::InvalidWildcard:
        emit errorOccurred(tr("%1 is not a usable wildcard. Use a form like "
                              "*.example.com, with no path.")
                                   .arg(shown));
        break;
    case Result::WildcardNeedsBypassMode:
        emit errorOccurred(tr("A wildcard such as %1 only works in the "
                              "Directly mode, where listed addresses are "
                              "not accessed via the VPN.")
                                   .arg(shown));
        break;
    case Result::WildcardUnsupportedHere:
        emit errorOccurred(tr("Wildcard sites such as %1 are supported only on the "
                              "desktop app with WireGuard/AmneziaWG.")
                                   .arg(shown));
        break;
    }
}

void IpSplitTunnelingUiController::removeSite(int index)
{
    auto modelIndex = m_ipSplitTunnelingModel->index(index);
    auto hostname = m_ipSplitTunnelingModel->data(modelIndex, IpSplitTunnelingModel::Roles::UrlRole).toString();
    if (m_ipSplitTunnelingController->removeSite(hostname)) {
        emit finished(tr("Site removed: %1").arg(hostname));
    }
}

void IpSplitTunnelingUiController::removeSites()
{
    m_ipSplitTunnelingController->removeSites();
    emit finished(tr("Site list cleared!"));
}

void IpSplitTunnelingUiController::importSites(const QString &fileName, bool replaceExisting)
{
    QByteArray jsonData;
    if (!SystemController::readFile(fileName, jsonData)) {
        emit errorOccurred(tr("Can't open file: %1").arg(fileName));
        return;
    }

    QString errorMessage;
    if (m_ipSplitTunnelingController->importSitesFromJson(jsonData, replaceExisting, errorMessage)) {
        if (errorMessage.isEmpty()) {
            emit finished(tr("Import completed"));
        } else {
            emit finished(tr("Import completed. %1").arg(errorMessage));
        }
    } else {
        emit errorOccurred(errorMessage);
    }
}

void IpSplitTunnelingUiController::exportSites(const QString &fileName)
{
    QByteArray jsonData = m_ipSplitTunnelingController->exportSitesToJson();
    if (!SystemController::saveFile(fileName, jsonData)) {
        qInfo() << "IpSplitTunnelingUiController::exportSites: save or share was cancelled or failed";
        return;
    }
    emit finished(tr("Export completed"));
}

void IpSplitTunnelingUiController::toggleSplitTunneling(bool enabled)
{
    m_ipSplitTunnelingController->toggleSplitTunneling(enabled);
    emit isSplitTunnelingEnabledChanged();
}

void IpSplitTunnelingUiController::setRouteMode(int routeMode)
{
    m_ipSplitTunnelingController->setRouteMode(static_cast<amnezia::RouteMode>(routeMode));
    emit routeModeChanged();
}

int IpSplitTunnelingUiController::getRouteMode() const
{
    return static_cast<int>(m_ipSplitTunnelingController->getRouteMode());
}

bool IpSplitTunnelingUiController::isSplitTunnelingEnabled() const
{
    return m_ipSplitTunnelingController->isSplitTunnelingEnabled();
}

void IpSplitTunnelingUiController::updateModel()
{
    m_ipSplitTunnelingModel->updateModel(m_ipSplitTunnelingController->getCurrentSites());
}
