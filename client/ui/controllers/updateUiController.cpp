#include "updateUiController.h"

#include "ui/controllers/marketplaceUpdateController.h"

UpdateUiController::UpdateUiController(UpdateController* updateController,
                                       MarketplaceUpdateController* marketplaceUpdateController,
                                       QObject *parent)
    : QObject(parent), m_updateController(updateController), m_marketplaceUpdateController(marketplaceUpdateController)
{
    if (m_updateController) {
        connect(m_updateController, &UpdateController::updateFound, this, &UpdateUiController::updateFound);
    }

    // On mobile the store is the trigger, so the drawer content is refreshed from there
    if (m_marketplaceUpdateController) {
        connect(m_marketplaceUpdateController, &MarketplaceUpdateController::updateAvailable, this,
                &UpdateUiController::updateFound);
    }
}

QString UpdateUiController::getHeaderText() const
{
    if (!m_updateController) {
        return QString();
    }

    const QString version = m_updateController->getVersion();
    const QString releaseDate = m_updateController->getReleaseDate();
    if (releaseDate.trimmed().isEmpty()) {
        return tr("New version released: %1").arg(version);
    }

    return tr("New version released: %1 (%2)").arg(version, releaseDate);
}

QString UpdateUiController::getChangelogText() const
{
    if (!m_updateController) {
        return QString();
    }

    const QString rawChangelog = m_updateController->getRawChangelogText();
    if (rawChangelog.isEmpty()) {
        return tr("Failed to load changelog text");
    }

    QStringList lines = rawChangelog.split("\n");
    QStringList filteredChangeLogText;
    bool add = false;
    QString osSection;

#ifdef Q_OS_WINDOWS
    osSection = "### Windows";
#elif defined(Q_OS_IOS)
    osSection = "### iOS";
#elif defined(Q_OS_MACOS)
    osSection = "### macOS";
#elif defined(Q_OS_ANDROID)
    osSection = "### Android";
#elif defined(Q_OS_LINUX)
    osSection = "### Linux";
#endif

    for (const QString &line : lines) {
        if (line.startsWith("### General")) {
            add = true;
        } else if (line.startsWith("### ") && line != osSection) {
            add = false;
        } else if (line == osSection) {
            add = true;
        }

        if (add) {
            filteredChangeLogText.append(line);
        }
    }

    return filteredChangeLogText.join("\n");
}

QString UpdateUiController::getVersion() const
{
    return m_updateController ? m_updateController->getVersion() : QString();
}

void UpdateUiController::checkForUpdates()
{
    if (m_updateController) {
        m_updateController->checkForUpdates();
    }
}

void UpdateUiController::runInstaller()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // There is no installer to run on mobile, the store handles the update itself
    if (m_marketplaceUpdateController) {
        m_marketplaceUpdateController->openStoreUrl();
    }
#else
    if (m_updateController) {
        m_updateController->runInstaller();
    }
#endif
}
