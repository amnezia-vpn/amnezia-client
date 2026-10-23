#include "updateUiController.h"

#include <QDate>
#include <QLocale>

UpdateUiController::UpdateUiController(UpdateController* updateController, QObject *parent)
    : QObject(parent), m_updateController(updateController)
{
    if (m_updateController) {
        connect(m_updateController, &UpdateController::updateFound, this, &UpdateUiController::updateFound);
        connect(m_updateController, &UpdateController::updateNotFound, this, &UpdateUiController::updateNotFound);
        connect(m_updateController, &UpdateController::updateCheckFailed, this, &UpdateUiController::updateCheckFailed);
        connect(m_updateController, &UpdateController::updateStateChanged, this, &UpdateUiController::updateStateChanged);
        connect(m_updateController, &UpdateController::updateCheckRunningChanged, this, &UpdateUiController::isCheckRunningChanged);
    }
}

QString UpdateUiController::getVersion() const
{
    return m_updateController ? m_updateController->getVersion() : QString();
}

QString UpdateUiController::getReleaseInfoText() const
{
    if (!m_updateController) {
        return QString();
    }

    const QString version = m_updateController->getVersion();
    const QDate releaseDate = QDate::fromString(m_updateController->getReleaseDate(), Qt::ISODate);
    if (!releaseDate.isValid()) {
        return version;
    }

    const QString dateText = QLocale().toString(releaseDate, QStringLiteral("d MMM"));
    return QStringLiteral("%1 · %2").arg(dateText, version);
}

QString UpdateUiController::getDescription() const
{
    return m_updateController ? m_updateController->getDescription() : QString();
}

QStringList UpdateUiController::getTags() const
{
    return m_updateController ? m_updateController->getTags() : QStringList();
}

QStringList UpdateUiController::getNewFeatures() const
{
    return m_updateController ? m_updateController->getNewFeatures() : QStringList();
}

QStringList UpdateUiController::getImprovements() const
{
    return m_updateController ? m_updateController->getImprovements() : QStringList();
}

QStringList UpdateUiController::getBugFixes() const
{
    return m_updateController ? m_updateController->getBugFixes() : QStringList();
}

int UpdateUiController::getUpdateState() const
{
    return m_updateController ? static_cast<int>(m_updateController->getUpdateState())
                              : static_cast<int>(UpdateState::State::Idle);
}

bool UpdateUiController::isStoreUpdate() const
{
    return m_updateController ? m_updateController->isStoreUpdate() : false;
}

bool UpdateUiController::isCheckRunning() const
{
    return m_updateController ? m_updateController->isUpdateCheckRunning() : false;
}

void UpdateUiController::checkForUpdates()
{
    if (m_updateController) {
        m_updateController->checkForUpdates();
    }
}

void UpdateUiController::update()
{
    if (m_updateController) {
        m_updateController->startUpdate();
    }
}

void UpdateUiController::install()
{
    if (m_updateController) {
        m_updateController->installUpdate();
    }
}

void UpdateUiController::retry()
{
    if (m_updateController) {
        m_updateController->startUpdate();
    }
}
