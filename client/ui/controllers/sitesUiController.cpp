#include "sitesUiController.h"

#include "systemController.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

SitesUiController::SitesUiController(SitesController* sitesController,
                                      SitesModel* sitesModel, QObject *parent)
    : QObject(parent),
      m_sitesController(sitesController),
      m_sitesModel(sitesModel)
{
    m_sitesModel->updateModel(m_sitesController->getCurrentSites());
}

void SitesUiController::addSite(QString hostname)
{
    if (m_sitesController->addSite(hostname)) {
        m_sitesModel->updateModel(m_sitesController->getCurrentSites());
        emit finished(tr("New site added: %1").arg(hostname));
    }
}

void SitesUiController::removeSite(int index)
{
    auto modelIndex = m_sitesModel->index(index);
    auto hostname = m_sitesModel->data(modelIndex, SitesModel::Roles::UrlRole).toString();
    if (m_sitesController->removeSite(hostname)) {
        m_sitesModel->updateModel(m_sitesController->getCurrentSites());
        emit finished(tr("Site removed: %1").arg(hostname));
    }
}

void SitesUiController::removeSites()
{
    m_sitesController->removeSites();
    m_sitesModel->updateModel(m_sitesController->getCurrentSites());
    emit finished(tr("Site list cleared!"));
}

void SitesUiController::importSites(const QString &fileName, bool replaceExisting)
{
    QByteArray jsonData;
    if (!SystemController::readFile(fileName, jsonData)) {
        emit errorOccurred(tr("Can't open file: %1").arg(fileName));
        return;
    }

    QString errorMessage;
    if (m_sitesController->importSitesFromJson(jsonData, replaceExisting, errorMessage)) {
        m_sitesModel->updateModel(m_sitesController->getCurrentSites());
        emit finished(tr("Import completed"));
    } else {
        emit errorOccurred(errorMessage);
    }
}

void SitesUiController::exportSites(const QString &fileName)
{
    QByteArray jsonData = m_sitesController->exportSitesToJson();
    SystemController::saveFile(fileName, QString::fromUtf8(jsonData));
    emit finished(tr("Export completed"));
}

void SitesUiController::toggleSplitTunneling(bool enabled)
{
    m_sitesController->toggleSplitTunneling(enabled);
    emit isTunnelingEnabledChanged();
}

void SitesUiController::setRouteMode(int routeMode)
{
    m_sitesController->setRouteMode(static_cast<amnezia::RouteMode>(routeMode));
    m_sitesModel->updateModel(m_sitesController->getCurrentSites());
    emit routeModeChanged();
}

int SitesUiController::getRouteMode() const
{
    return static_cast<int>(m_sitesController->getRouteMode());
}

bool SitesUiController::isTunnelingEnabled() const
{
    return m_sitesController->isSplitTunnelingEnabled();
}

void SitesUiController::updateModel()
{
    m_sitesModel->updateModel(m_sitesController->getCurrentSites());
}
