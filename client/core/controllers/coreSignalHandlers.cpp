#include "coreSignalHandlers.h"

#include <QTimer>

#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/defs.h"
#include "core/controllers/coreController.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "vpnconnection.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/controllers/connectionUiController.h"
#include "ui/controllers/settingsUiController.h"
#include "ui/controllers/serversUiController.h"
#include "ui/controllers/sitesUiController.h"
#include "ui/controllers/allowedDnsUiController.h"
#include "ui/controllers/appSplitTunnelingUiController.h"
#include "ui/controllers/languageUiController.h"
#include "ui/controllers/selfhosted/installUiController.h"
#include "ui/controllers/importUiController.h"
#include "ui/controllers/api/subscriptionUiController.h"
#include "ui/controllers/selfhosted/protocolsUiController.h"
#include "ui/models/serversModel.h"
#include "core/controllers/serversController.h"
#include "core/controllers/sitesController.h"
#include "core/controllers/appSplitTunnelingController.h"
#include "core/controllers/selfhosted/usersController.h"
#include "core/controllers/settingsController.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/exportController.h"
#include "core/controllers/connectionController.h"
#include "ui/models/clientManagementModel.h"
#include "ui/controllers/api/apiNewsUiController.h"
#include "ui/models/api/apiCountryModel.h"

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
    initApiCountryModelUpdateHandler();
    initContainerModelUpdateHandler();
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
    initStrictKillSwitchHandler();
    initAndroidSettingsHandler();
    initAndroidConnectionHandler();
    initIosImportHandler();
    initIosSettingsHandler();
    initNotificationHandler();
}

void CoreSignalHandlers::initErrorMessagesHandler()
{
    connect(m_coreController->m_connectionUiController, &ConnectionUiController::connectionErrorOccurred, this, [this](ErrorCode errorCode) {
        emit m_coreController->m_pageController->showErrorMessage(errorCode);
        emit m_coreController->m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
    });

    connect(m_coreController->m_subscriptionUiController, &SubscriptionUiController::errorOccurred, m_coreController->m_pageController,
            qOverload<ErrorCode>(&PageController::showErrorMessage));
}

void CoreSignalHandlers::initSettingsSplitTunnelingHandler()
{
    connect(m_coreController->m_settingsController, &SettingsController::siteSplitTunnelingRouteModeChanged, this, [this](RouteMode mode) {
        m_coreController->m_sitesController->setRouteMode(mode);
    });
    connect(m_coreController->m_settingsController, &SettingsController::siteSplitTunnelingToggled, this, [this](bool enabled) {
        m_coreController->m_sitesController->toggleSplitTunneling(enabled);
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
    connect(m_coreController->m_installUiController, &InstallUiController::currentContainerUpdated, m_coreController->m_connectionUiController,
            &ConnectionUiController::onCurrentContainerUpdated);
    connect(m_coreController->m_installUiController, &InstallUiController::profileCleared,
            m_coreController->m_protocolsUiController, &ProtocolsUiController::updateProtocols);
}

void CoreSignalHandlers::initExportControllerHandler()
{
    connect(m_coreController->m_exportController, &ExportController::appendClientRequested, this,
            [this](int serverIndex, const QString &clientId, const QString &clientName, DockerContainer container) {
                m_coreController->m_usersController->appendClient(serverIndex, clientId, clientName, container);
            });
    connect(m_coreController->m_exportController, &ExportController::updateClientsRequested, this,
            [this](int serverIndex, DockerContainer container) {
                m_coreController->m_usersController->updateClients(serverIndex, container);
            });
    connect(m_coreController->m_exportController, &ExportController::revokeClientRequested, this,
            [this](int serverIndex, int row, DockerContainer container) {
                m_coreController->m_usersController->revokeClient(serverIndex, row, container);
            });
    connect(m_coreController->m_exportController, &ExportController::renameClientRequested, this,
            [this](int serverIndex, int row, const QString &clientName, DockerContainer container) {
                m_coreController->m_usersController->renameClient(serverIndex, row, clientName, container);
            });
}

void CoreSignalHandlers::initApiCountryModelUpdateHandler()
{
    connect(m_coreController->m_serversModel, &ServersModel::updateApiCountryModel, this, [this]() {
        m_coreController->m_apiCountryModel->updateModel(m_coreController->m_serversModel->getProcessedServerData("apiAvailableCountries").toJsonArray(),
                                       m_coreController->m_serversModel->getProcessedServerData("apiServerCountryCode").toString());
    });
}

void CoreSignalHandlers::initContainerModelUpdateHandler()
{
    connect(m_coreController->m_serversController, &ServersController::gatewayStacksExpanded, this, [this]() {
        if (m_coreController->m_serversUiController->hasServersFromGatewayApi()) {
            m_coreController->m_apiNewsUiController->fetchNews(false);
        }
    });
}

void CoreSignalHandlers::initAdminConfigRevokedHandler()
{
    connect(m_coreController->m_installController, &InstallController::clientRevocationRequested, this,
            [this](int serverIndex, const QJsonObject &containerConfig, DockerContainer container) {
                m_coreController->m_usersController->revokeClient(serverIndex, containerConfig, container);
            });

    connect(m_coreController->m_installController, &InstallController::clientAppendRequested, this,
            [this](int serverIndex, const QString &clientId, const QString &clientName, DockerContainer container) {
                m_coreController->m_usersController->appendClient(serverIndex, clientId, clientName, container);
            });
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
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::appLanguageChanged, m_coreController->m_languageUiController, &LanguageUiController::onAppLanguageChanged);
    connect(m_coreController->m_settingsUiController, &SettingsUiController::resetLanguageToSystem, m_coreController->m_languageUiController, [this]() {
        m_coreController->m_languageUiController->changeLanguage(m_coreController->m_languageUiController->getSystemLanguageEnum());
    });
}

void CoreSignalHandlers::initAutoConnectHandler()
{
    if (m_coreController->m_settingsUiController->isAutoConnectEnabled() && m_coreController->m_serversController->getDefaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this]() { m_coreController->m_connectionUiController->openConnection(); });
    }
}

void CoreSignalHandlers::initAmneziaDnsToggledHandler()
{
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::useAmneziaDnsChanged, m_coreController->m_serversUiController, [this](bool enabled) {
        Q_UNUSED(enabled);
        m_coreController->m_serversUiController->updateModel();
    });
}

void CoreSignalHandlers::initServersModelUpdateHandler()
{
    connect(m_coreController->m_serversRepository, &QServersRepository::serverAdded,
            m_coreController->m_serversUiController, &ServersUiController::onAddServer);
    connect(m_coreController->m_serversRepository, &QServersRepository::serverEdited,
            m_coreController->m_serversUiController, &ServersUiController::onServerEdited);
    connect(m_coreController->m_serversRepository, &QServersRepository::serverRemoved,
            m_coreController->m_serversUiController, &ServersUiController::onServerRemoved);
    connect(m_coreController->m_serversRepository, &QServersRepository::defaultServerChanged,
            m_coreController->m_serversUiController, &ServersUiController::onDefaultServerChanged);
    
    connect(m_coreController->m_serversRepository, &QServersRepository::serverAdded,
            m_coreController->m_serversController, &ServersController::recomputeGatewayStacks);
    connect(m_coreController->m_serversRepository, &QServersRepository::serverEdited,
            m_coreController->m_serversController, &ServersController::recomputeGatewayStacks);
    connect(m_coreController->m_serversRepository, &QServersRepository::serverRemoved,
            m_coreController->m_serversController, &ServersController::recomputeGatewayStacks);
}

void CoreSignalHandlers::initClientManagementModelUpdateHandler()
{
    connect(m_coreController->m_usersController, &UsersController::clientsUpdated,
            m_coreController->m_clientManagementModel, &ClientManagementModel::updateModel);
}

void CoreSignalHandlers::initSitesModelUpdateHandler()
{
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::sitesChanged, m_coreController->m_sitesUiController, [this](amnezia::RouteMode mode) {
        Q_UNUSED(mode);
        m_coreController->m_sitesUiController->updateModel();
    });
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::sitesSplitTunnelingEnabledChanged, m_coreController->m_sitesUiController, [this](bool enabled) {
        Q_UNUSED(enabled);
        m_coreController->m_sitesUiController->updateModel();
    });
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::routeModeChanged, m_coreController->m_sitesUiController, [this](amnezia::RouteMode mode) {
        Q_UNUSED(mode);
        m_coreController->m_sitesUiController->updateModel();
    });
}

void CoreSignalHandlers::initAllowedDnsModelUpdateHandler()
{
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::allowedDnsServersChanged, m_coreController->m_allowedDnsUiController, [this](const QStringList &servers) {
        Q_UNUSED(servers);
        m_coreController->m_allowedDnsUiController->updateModel();
    });
}

void CoreSignalHandlers::initAppSplitTunnelingModelUpdateHandler()
{
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::appsChanged, m_coreController->m_appSplitTunnelingUiController, [this](amnezia::AppsRouteMode mode) {
        Q_UNUSED(mode);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
    });
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::appsSplitTunnelingEnabledChanged, m_coreController->m_appSplitTunnelingUiController, [this](bool enabled) {
        Q_UNUSED(enabled);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
    });
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::appsRouteModeChanged, m_coreController->m_appSplitTunnelingUiController, [this](amnezia::AppsRouteMode mode) {
        Q_UNUSED(mode);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
    });
}

void CoreSignalHandlers::initPrepareConfigHandler()
{
    connect(m_coreController->m_connectionUiController, &ConnectionUiController::prepareConfig, this, [this]() {
        emit m_coreController->m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);

        if (!m_coreController->m_subscriptionUiController->isConfigValid()) {
            emit m_coreController->m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        if (!m_coreController->m_installUiController->isConfigValid()) {
            emit m_coreController->m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        m_coreController->m_connectionUiController->openConnection();
    });
}

void CoreSignalHandlers::initStrictKillSwitchHandler()
{
    connect(m_coreController->m_settingsUiController, &SettingsUiController::strictKillSwitchEnabledChanged, m_coreController->m_vpnConnection.get(),
            &VpnConnection::onKillSwitchModeChanged);
}

void CoreSignalHandlers::initAndroidSettingsHandler()
{
#ifdef Q_OS_ANDROID
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::saveLogsChanged, AndroidController::instance(), &AndroidController::setSaveLogs);
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::screenshotsEnabledChanged, AndroidController::instance(), &AndroidController::setScreenshotsEnabled);
    connect(m_coreController->m_serversRepository, &QServersRepository::serverRemoved, AndroidController::instance(), &AndroidController::resetLastServer);
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::settingsCleared, []() { AndroidController::instance()->resetLastServer(-1); });
#endif
}

void CoreSignalHandlers::initAndroidConnectionHandler()
{
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::initConnectionState, this, [this](Vpn::ConnectionState state) {
        m_coreController->m_connectionUiController->onConnectionStateChanged(state);
        if (m_coreController->m_vpnConnection)
            m_coreController->m_vpnConnection->restoreConnection();
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
    connect(m_coreController->m_appSettingsRepository, &QAppSettingsRepository::screenshotsEnabledChanged, [](bool enabled) { AmneziaVPN::toggleScreenshots(enabled); });
#endif
}

void CoreSignalHandlers::initNotificationHandler()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    m_coreController->m_notificationHandler = NotificationHandler::create(m_coreController);

    connect(m_coreController->m_vpnConnection.get(), &VpnConnection::connectionStateChanged, m_coreController->m_notificationHandler,
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

