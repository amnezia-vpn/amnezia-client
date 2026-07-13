#include "coreSignalHandlers.h"

#include <QTimer>
#include <QtConcurrent>

#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/controllers/coreController.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/serverConfigUtils.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "vpnConnection.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/controllers/connectionUiController.h"
#include "ui/controllers/settingsUiController.h"
#include "ui/controllers/serversUiController.h"
#include "ui/controllers/ipSplitTunnelingUiController.h"
#include "ui/controllers/allowedDnsUiController.h"
#include "ui/controllers/appSplitTunnelingUiController.h"
#include "ui/controllers/languageUiController.h"
#include "ui/controllers/selfhosted/installUiController.h"
#include "ui/controllers/importUiController.h"
#include "ui/controllers/api/subscriptionUiController.h"
#include "ui/controllers/updateUiController.h"
#include "ui/models/serversModel.h"
#include "core/controllers/serversController.h"
#include "core/controllers/ipSplitTunnelingController.h"
#include "core/controllers/appSplitTunnelingController.h"
#include "core/controllers/selfhosted/usersController.h"
#include "core/controllers/settingsController.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/exportController.h"
#include "core/controllers/connectionController.h"
#include "ui/models/clientManagementModel.h"
#include "ui/controllers/api/apiNewsUiController.h"
#include "ui/models/containersModel.h"
#include "core/utils/containerEnum.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    #include "ui/utils/notificationHandler.h"
    #include "ui/utils/systemTrayNotificationHandler.h"
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
    #include <AmneziaVPN-Swift.h>
#endif

CoreSignalHandlers::CoreSignalHandlers(CoreController* coreController, QObject* parent)
    : QObject(parent),
      m_coreController(coreController)
{
}

void CoreSignalHandlers::initAllHandlers()
{
    initErrorMessagesHandler();
    initSettingsSplitTunnelingHandler();
    initInstallControllerHandler();
    initExportControllerHandler();
    initImportControllerHandler();
    initApiCountryModelUpdateHandler();
    initSubscriptionRefreshHandler();
    initAdminConfigRevokedHandler();
    initPassphraseRequestHandler();
    initTranslationsUpdatedHandler();
    initLanguageHandler();
    initAutoConnectHandler();
    initAmneziaDnsToggledHandler();
    initServersModelUpdateHandler();
    initClientManagementModelUpdateHandler();
    initSitesModelUpdateHandler();
    initAllowedDnsModelUpdateHandler();
    initAppSplitTunnelingModelUpdateHandler();
    initPrepareConfigHandler();
    initUnsupportedConnectDrawerHandler();
    initStrictKillSwitchHandler();
    initAndroidSettingsHandler();
    initAndroidConnectionHandler();
    initIosImportHandler();
    initIosSettingsHandler();
    initNotificationHandler();
    initUpdateFoundHandler();
}

void CoreSignalHandlers::initErrorMessagesHandler()
{
    connect(m_coreController->m_connectionUiController, &ConnectionUiController::connectionErrorOccurred, this, [this](ErrorCode errorCode) {
        emit m_coreController->m_pageController->showErrorMessage(errorCode);
        m_coreController->m_connectionController->setConnectionState(Vpn::ConnectionState::Disconnected);
    });

    connect(m_coreController->m_subscriptionUiController, &SubscriptionUiController::errorOccurred, m_coreController->m_pageController,
            qOverload<ErrorCode>(&PageController::showErrorMessage));

    connect(m_coreController->m_settingsUiController, &SettingsUiController::errorOccurred, m_coreController->m_pageController,
            qOverload<ErrorCode>(&PageController::showErrorMessage));
}

void CoreSignalHandlers::initSettingsSplitTunnelingHandler()
{
    connect(m_coreController->m_settingsController, &SettingsController::siteSplitTunnelingRouteModeChanged, this, [this](RouteMode mode) {
        m_coreController->m_ipSplitTunnelingController->setRouteMode(mode);
    });
    connect(m_coreController->m_settingsController, &SettingsController::siteSplitTunnelingToggled, this, [this](bool enabled) {
        m_coreController->m_ipSplitTunnelingController->toggleSplitTunneling(enabled);
    });
    connect(m_coreController->m_settingsController, &SettingsController::appSplitTunnelingRouteModeChanged, this, [this](AppsRouteMode mode) {
        m_coreController->m_appSplitTunnelingController->setRouteMode(mode);
    });
    connect(m_coreController->m_settingsController, &SettingsController::appSplitTunnelingToggled, this, [this](bool enabled) {
        m_coreController->m_appSplitTunnelingController->toggleSplitTunneling(enabled);
    });
    connect(m_coreController->m_settingsController, &SettingsController::appSplitTunnelingClearAppsList, this, [this]() {
        m_coreController->m_appSplitTunnelingController->clearAppsList();
    });
}

void CoreSignalHandlers::initInstallControllerHandler()
{
    connect(m_coreController->m_installController, &InstallController::serverIsBusy, m_coreController->m_installUiController, &InstallUiController::serverIsBusy);
    connect(m_coreController->m_installUiController, &InstallUiController::cancelInstallation, m_coreController->m_installController, &InstallController::cancelInstallation);
    connect(m_coreController->m_serversUiController, &ServersUiController::processedServerIdChanged,
        m_coreController->m_installUiController, [this](const QString &serverId) {
        if (!serverId.isEmpty()) {
            m_coreController->m_installUiController->clearProcessedServerCredentials();
        }
    });
}

void CoreSignalHandlers::initExportControllerHandler()
{
    connect(m_coreController->m_exportController, &ExportController::appendClientRequested, this,
            [this](const QString &serverId, const QString &clientId, const QString &clientName, DockerContainer container) {
                m_coreController->m_usersController->appendClient(serverId, clientId, clientName, container);
            });
    connect(m_coreController->m_exportController, &ExportController::updateClientsRequested, this,
            [this](const QString &serverId, DockerContainer container) {
                m_coreController->m_usersController->updateClients(serverId, container);
            });
    connect(m_coreController->m_exportController, &ExportController::revokeClientRequested, this,
            [this](const QString &serverId, int row, DockerContainer container) {
                QtConcurrent::run([this, serverId, row, container]() {
                    m_coreController->m_usersController->revokeClient(serverId, row, container);
                });
            });
    connect(m_coreController->m_exportController, &ExportController::renameClientRequested, this,
            [this](const QString &serverId, int row, const QString &clientName, DockerContainer container) {
                m_coreController->m_usersController->renameClient(serverId, row, clientName, container);
            });
}

void CoreSignalHandlers::initImportControllerHandler()
{
    connect(m_coreController->m_importCoreController, &ImportController::importFinished, this, [this]() {
        if (m_coreController->m_connectionUiController->isConnected()) {
            return;
        }

        const int newServerIndex = m_coreController->m_serversController->getServersCount() - 1;
        const QString serverId = m_coreController->m_serversController->getServerId(newServerIndex);
        if (!serverId.isEmpty()) {
            m_coreController->m_serversController->setDefaultServer(serverId);
        }
        if (m_coreController->m_serversUiController) {
            m_coreController->m_serversUiController->setProcessedServerId(serverId);
        }
    });
}

void CoreSignalHandlers::initApiCountryModelUpdateHandler()
{
    connect(m_coreController->m_serversUiController, &ServersUiController::updateApiCountryModel, this, [this]() {
        const QString processedServerId = m_coreController->m_serversUiController->getProcessedServerId();
        if (processedServerId.isEmpty()) {
            return;
        }

        const auto apiV2 = m_coreController->m_serversRepository->apiV2Config(processedServerId);
        if (!apiV2.has_value()) {
            return;
        }

        m_coreController->m_apiCountryModel->updateModel(apiV2->apiConfig.availableCountries,
                                                           apiV2->apiConfig.serverCountryCode);
    });
}

void CoreSignalHandlers::initSubscriptionRefreshHandler()
{
    connect(m_coreController->m_subscriptionUiController, &SubscriptionUiController::subscriptionRefreshNeeded, this, [this]() {
        const QString defaultServerId = m_coreController->m_serversController->getDefaultServerId();
        if (!defaultServerId.isEmpty()) {
            m_coreController->m_subscriptionUiController->getAccountInfo(defaultServerId, false);
        }
    });
}

void CoreSignalHandlers::initAdminConfigRevokedHandler()
{
    connect(m_coreController->m_installController, &InstallController::clientRevocationRequested, this,
            [this](const QString &serverId, const ContainerConfig &containerConfig, DockerContainer container) {
                QtConcurrent::run([this, serverId, containerConfig, container]() {
                    m_coreController->m_usersController->revokeClient(serverId, containerConfig, container);
                });
            });

    connect(m_coreController->m_installController, &InstallController::clientAppendRequested, this,
            [this](const QString &serverId, const QString &clientId, const QString &clientName, DockerContainer container) {
                m_coreController->m_usersController->appendClient(serverId, clientId, clientName, container);
            }, Qt::DirectConnection);

    connect(m_coreController->m_usersController, &UsersController::adminConfigRevoked, m_coreController->m_installController,
            &InstallController::clearCachedProfile);
}

void CoreSignalHandlers::initPassphraseRequestHandler()
{
    connect(m_coreController->m_installUiController, &InstallUiController::passphraseRequestStarted, m_coreController->m_pageController,
            &PageController::showPassphraseRequestDrawer);
    connect(m_coreController->m_pageController, &PageController::passphraseRequestDrawerClosed, m_coreController->m_installUiController,
            &InstallUiController::setEncryptedPassphrase);
}

void CoreSignalHandlers::initTranslationsUpdatedHandler()
{
    connect(m_coreController->m_languageUiController, &LanguageUiController::updateTranslations, m_coreController, &CoreController::updateTranslator);
    connect(m_coreController, &CoreController::translationsUpdated, m_coreController->m_languageUiController, &LanguageUiController::translationsUpdated);
    connect(m_coreController, &CoreController::translationsUpdated, m_coreController->m_connectionUiController, &ConnectionUiController::onTranslationsUpdated);
}

void CoreSignalHandlers::initLanguageHandler()
{
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::appLanguageChanged, m_coreController->m_languageUiController, &LanguageUiController::onAppLanguageChanged);
    connect(m_coreController->m_settingsUiController, &SettingsUiController::resetLanguageToSystem, m_coreController->m_languageUiController, [this]() {
        m_coreController->m_languageUiController->changeLanguage(m_coreController->m_languageUiController->getSystemLanguageEnum());
    });
    connect(m_coreController->m_settingsUiController, &SettingsUiController::appLanguageChanged, m_coreController->m_languageUiController, [this]() {
        m_coreController->m_languageUiController->onAppLanguageChanged(m_coreController->m_settingsController->getAppLanguage());
    });
}

void CoreSignalHandlers::initAutoConnectHandler()
{
    if (m_coreController->m_settingsUiController->isAutoConnectEnabled()
        && !m_coreController->m_serversController->getDefaultServerId().isEmpty()) {
        QTimer::singleShot(1000, this, [this]() { m_coreController->m_connectionUiController->toggleConnection(); });
    }
}

void CoreSignalHandlers::initAmneziaDnsToggledHandler()
{
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::useAmneziaDnsChanged, m_coreController->m_serversUiController, &ServersUiController::updateModel);
}

void CoreSignalHandlers::initServersModelUpdateHandler()
{
    connect(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded,
            m_coreController->m_serversUiController, &ServersUiController::updateModel);
    connect(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited,
            m_coreController->m_serversUiController, &ServersUiController::updateModel);
    connect(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved,
            m_coreController->m_serversUiController, &ServersUiController::updateModel);
    connect(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged,
            m_coreController->m_serversUiController, &ServersUiController::onDefaultServerChanged);

    connect(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded, this,
            [this](const QString &serverId) {
                if (m_coreController->m_serversRepository->apiV2Config(serverId).has_value()) {
                    m_coreController->m_apiNewsUiController->fetchNews(false);
                }
            });

    connect(m_coreController->m_settingsUiController, &SettingsUiController::restoreBackupFinished, this, [this]() {
        m_coreController->m_serversUiController->updateModel();
        if (m_coreController->m_serversUiController->hasServersFromGatewayApi()) {
            m_coreController->m_apiNewsUiController->fetchNews(false);
        }
    });
}

void CoreSignalHandlers::initClientManagementModelUpdateHandler()
{
    connect(m_coreController->m_usersController, &UsersController::clientsUpdated,
            m_coreController->m_clientManagementModel, &ClientManagementModel::updateModel);
    connect(m_coreController->m_usersController, &UsersController::clientRenamed,
            m_coreController->m_clientManagementModel, &ClientManagementModel::updateClientName);
    connect(m_coreController->m_usersController, &UsersController::revokeFinished,
            m_coreController->m_exportController, &ExportController::revokeFinished);
}

void CoreSignalHandlers::initSitesModelUpdateHandler()
{
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::sitesChanged, m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::updateModel);
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::sitesSplitTunnelingEnabledChanged, m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::updateModel);
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::routeModeChanged, m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::updateModel);
}

void CoreSignalHandlers::initAllowedDnsModelUpdateHandler()
{
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::allowedDnsServersChanged, m_coreController->m_allowedDnsUiController, &AllowedDnsUiController::updateModel);
}

void CoreSignalHandlers::initAppSplitTunnelingModelUpdateHandler()
{
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::appsChanged, m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::updateModel);
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::appsSplitTunnelingEnabledChanged, m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::updateModel);
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::appsRouteModeChanged, m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::updateModel);
}

void CoreSignalHandlers::initPrepareConfigHandler()
{
    connect(m_coreController->m_connectionUiController, &ConnectionUiController::prepareConfig, this, [this]() {
        m_coreController->m_connectionController->setConnectionState(Vpn::ConnectionState::Preparing);

        const QString serverId = m_coreController->m_serversController->getDefaultServerId();
        if (serverId.isEmpty()) {
            m_coreController->m_connectionController->setConnectionState(Vpn::ConnectionState::Disconnected);
            return;
        }

        const serverConfigUtils::ConfigType kind = m_coreController->m_serversRepository->serverKind(serverId);

        if (serverConfigUtils::isApiV2Subscription(kind) || serverConfigUtils::isLegacyApiSubscription(kind)) {
            m_coreController->m_subscriptionUiController->validateConfig();
        } else {
            m_coreController->m_installUiController->validateConfig();
        }
    });

    connect(m_coreController->m_subscriptionUiController, &SubscriptionUiController::configValidated, this, [this](bool isValid) {
        if (!isValid) {
            m_coreController->m_connectionController->setConnectionState(Vpn::ConnectionState::Disconnected);
            return;
        }

        m_coreController->m_connectionUiController->openConnection();
    });

    connect(m_coreController->m_installUiController, &InstallUiController::configValidated, this, [this](bool isValid) {
        if (!isValid) {
            m_coreController->m_connectionController->setConnectionState(Vpn::ConnectionState::Disconnected);
            return;
        }

        m_coreController->m_connectionUiController->openConnection();
    });
}

void CoreSignalHandlers::initUnsupportedConnectDrawerHandler()
{
    connect(m_coreController->m_subscriptionUiController, &SubscriptionUiController::unsupportedConnectDrawerRequested,
            m_coreController->m_pageController, &PageController::unsupportedConnectDrawerRequested);

    connect(m_coreController->m_connectionUiController, &ConnectionUiController::unsupportedConnectDrawerRequested,
            m_coreController->m_pageController, &PageController::unsupportedConnectDrawerRequested);
}

void CoreSignalHandlers::initStrictKillSwitchHandler()
{
    connect(m_coreController->m_settingsUiController, &SettingsUiController::strictKillSwitchEnabledChanged, m_coreController->m_connectionController,
            &ConnectionController::onKillSwitchModeChanged);
}

void CoreSignalHandlers::initAndroidSettingsHandler()
{
#ifdef Q_OS_ANDROID
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::saveLogsChanged, AndroidController::instance(), &AndroidController::setSaveLogs);
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::screenshotsEnabledChanged, AndroidController::instance(), &AndroidController::setScreenshotsEnabled);
    connect(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved, this,
            [](const QString &/*serverId*/, int removedIndex) {
                AndroidController::instance()->resetLastServer(removedIndex);
            });
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::settingsCleared, []() { AndroidController::instance()->resetLastServer(-1); });
#endif
}

void CoreSignalHandlers::initAndroidConnectionHandler()
{
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::initConnectionState, this, [this](Vpn::ConnectionState state) {
        m_coreController->m_connectionUiController->onConnectionStateChanged(state);
        m_coreController->m_connectionController->restoreConnection();
    });
    connect(AndroidController::instance(), &AndroidController::importConfigFromOutside, this, [this](QString data) {
        emit m_coreController->m_pageController->goToPageHome();
        m_coreController->m_importController->extractConfigFromData(data);
        data.clear();
        emit m_coreController->m_pageController->goToPageViewConfig();
    });
#endif
}

void CoreSignalHandlers::initIosImportHandler()
{
#ifdef Q_OS_IOS
    connect(IosController::Instance(), &IosController::importConfigFromOutside, this, [this](QString data) {
        emit m_coreController->m_pageController->goToPageHome();
        m_coreController->m_importController->extractConfigFromData(data);
        emit m_coreController->m_pageController->goToPageViewConfig();
    });
    connect(IosController::Instance(), &IosController::importBackupFromOutside, this, [this](QString filePath) {
        emit m_coreController->m_pageController->goToPageHome();
        m_coreController->m_pageController->goToPageSettingsBackup();
        emit m_coreController->m_settingsUiController->importBackupFromOutside(filePath);
    });
#endif
}

void CoreSignalHandlers::initIosSettingsHandler()
{
#ifdef Q_OS_IOS
    connect(m_coreController->m_appSettingsRepository, &SecureAppSettingsRepository::screenshotsEnabledChanged, [](bool enabled) { AmneziaVPN::toggleScreenshots(enabled); });
#endif
}

void CoreSignalHandlers::initNotificationHandler()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    m_coreController->m_notificationHandler = NotificationHandler::create(m_coreController);

    connect(m_coreController->m_connectionController, &ConnectionController::connectionStateChanged, m_coreController->m_notificationHandler,
            &NotificationHandler::setConnectionState);

    connect(m_coreController->m_notificationHandler, &NotificationHandler::raiseRequested, m_coreController->m_pageController, &PageController::raiseMainWindow);
    connect(m_coreController->m_notificationHandler, &NotificationHandler::connectRequested, m_coreController->m_connectionUiController,
            static_cast<void (ConnectionUiController::*)()>(&ConnectionUiController::openConnection));
    connect(m_coreController->m_notificationHandler, &NotificationHandler::disconnectRequested, m_coreController->m_connectionUiController,
            &ConnectionUiController::closeConnection);
    connect(m_coreController, &CoreController::translationsUpdated, m_coreController->m_notificationHandler, &NotificationHandler::onTranslationsUpdated);

    auto* trayHandler = qobject_cast<SystemTrayNotificationHandler*>(m_coreController->m_notificationHandler);
    connect(m_coreController, &CoreController::websiteUrlChanged, trayHandler, &SystemTrayNotificationHandler::updateWebsiteUrl);
#endif    
}

void CoreSignalHandlers::initUpdateFoundHandler()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    connect(m_coreController->m_apiNewsUiController, &ApiNewsUiController::fetchNewsFinished, m_coreController->m_updateUiController,
            &UpdateUiController::checkForUpdates);

    connect(m_coreController->m_updateUiController, &UpdateUiController::updateFound, this, [this]() {
        const QString version = m_coreController->m_updateUiController->getVersion();
        const QString updateId = version.isEmpty() ? QStringLiteral("update") : QStringLiteral("update-%1").arg(version);
        m_coreController->m_newsModel->setUpdateNotification(
                updateId, m_coreController->m_updateUiController->getHeaderText(), m_coreController->m_updateUiController->getChangelogText());
        emit m_coreController->m_pageController->showChangelogDrawer();
    });
#endif
}

