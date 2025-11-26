#include "coreController.h"

#include <QDirIterator>
#include <QTranslator>

#if defined(Q_OS_ANDROID)
    #include "core/installedAppsImageProvider.h"
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/ios_controller.h"
    #include <AmneziaVPN-Swift.h>
#endif

CoreController::CoreController(const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                               QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent), m_vpnConnection(vpnConnection), m_settings(settings), m_engine(engine)
{
    initRepositories();
    initCoreControllers();
    initModels();
    initControllers();
    initSignalHandlers();

    initAndroidController();
    initAppleController();

    initNotificationHandler();

    m_translator = new QTranslator(this);
    updateTranslator(m_appSettingsRepository->getAppLanguage());
}

void CoreController::initModels()
{
    m_containersModel = new ContainersModel(this);
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel);

    m_defaultServerContainersModel = new ContainersModel(this);
    m_engine->rootContext()->setContextProperty("DefaultServerContainersModel", m_defaultServerContainersModel);

    m_serversModel = new ServersModel(this);
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel);

    m_languageModel = new LanguageModel(this);
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel);

    m_sitesModel = new SitesModel(this);
    m_engine->rootContext()->setContextProperty("SitesModel", m_sitesModel);

    m_allowedDnsModel = new AllowedDnsModel(this);
    m_engine->rootContext()->setContextProperty("AllowedDnsModel", m_allowedDnsModel);

    m_appSplitTunnelingModel = new AppSplitTunnelingModel(this);
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingModel", m_appSplitTunnelingModel);

    m_protocolsModel = new ProtocolsModel(this);
    m_engine->rootContext()->setContextProperty("ProtocolsModel", m_protocolsModel);
    
    m_protocolsUiController = new ProtocolsUiController(m_protocolsModel, this);
    m_engine->rootContext()->setContextProperty("ProtocolsUiController", m_protocolsUiController);

    m_openVpnConfigModel = new OpenVpnConfigModel(this);
    m_engine->rootContext()->setContextProperty("OpenVpnConfigModel", m_openVpnConfigModel);

    m_shadowSocksConfigModel = new ShadowSocksConfigModel(this);
    m_engine->rootContext()->setContextProperty("ShadowSocksConfigModel", m_shadowSocksConfigModel);

    m_cloakConfigModel = new CloakConfigModel(this);
    m_engine->rootContext()->setContextProperty("CloakConfigModel", m_cloakConfigModel);

    m_wireGuardConfigModel = new WireGuardConfigModel(this);
    m_engine->rootContext()->setContextProperty("WireGuardConfigModel", m_wireGuardConfigModel);

    m_awgConfigModel = new AwgConfigModel(this);
    m_engine->rootContext()->setContextProperty("AwgConfigModel", m_awgConfigModel);

    m_xrayConfigModel = new XrayConfigModel(this);
    m_engine->rootContext()->setContextProperty("XrayConfigModel", m_xrayConfigModel);

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel = new Ikev2ConfigModel(this);
    m_engine->rootContext()->setContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel);
#endif

    m_sftpConfigModel = new SftpConfigModel(this);
    m_engine->rootContext()->setContextProperty("SftpConfigModel", m_sftpConfigModel);

    m_socks5ConfigModel = new Socks5ProxyConfigModel(this);
    m_engine->rootContext()->setContextProperty("Socks5ProxyConfigModel", m_socks5ConfigModel);

    m_clientManagementModel = new ClientManagementModel(this);
    m_engine->rootContext()->setContextProperty("ClientManagementModel", m_clientManagementModel);

    m_apiServicesModel = new ApiServicesModel(this);
    m_engine->rootContext()->setContextProperty("ApiServicesModel", m_apiServicesModel);

    m_apiCountryModel = new ApiCountryModel(this);
    m_engine->rootContext()->setContextProperty("ApiCountryModel", m_apiCountryModel);

    m_apiAccountInfoModel = new ApiAccountInfoModel(this);
    m_engine->rootContext()->setContextProperty("ApiAccountInfoModel", m_apiAccountInfoModel);

    m_apiDevicesModel = new ApiDevicesModel(this);
    m_engine->rootContext()->setContextProperty("ApiDevicesModel", m_apiDevicesModel);

    m_newsModel = new NewsModel(m_settings, this);
    m_engine->rootContext()->setContextProperty("NewsModel", m_newsModel);
}

void CoreController::initRepositories()
{
    m_serversRepository = new QServersRepository(m_settings, this);
    m_appSettingsRepository = new QAppSettingsRepository(m_settings, this);
}

void CoreController::initCoreControllers()
{
    ServersRepository* serversRepo = m_serversRepository->repository();
    AppSettingsRepository* appSettingsRepo = m_appSettingsRepository->repository();
    
    m_serversController = new ServersController(serversRepo, appSettingsRepo, this);
    m_appSplitTunnelingController = new AppSplitTunnelingController(appSettingsRepo);
    m_clientManagementController = new ClientManagementController(serversRepo, this);
    m_sitesController = new SitesController(appSettingsRepo);
    m_allowedDnsController = new AllowedDnsController(appSettingsRepo);
    m_servicesCatalogController = new ServicesCatalogController(appSettingsRepo);
    m_subscriptionController = new SubscriptionController(serversRepo, appSettingsRepo);
}

void CoreController::initControllers()
{
    m_connectionController = new ConnectionController(m_serversController, m_serversModel, m_containersModel, m_clientManagementModel, m_vpnConnection.get(), m_appSettingsRepository, m_settings, this);
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController);

    m_pageController = new PageController(m_serversModel, m_appSettingsRepository, this);
    m_engine->rootContext()->setContextProperty("PageController", m_pageController);

    m_focusController = new FocusController(m_engine, this);
    m_engine->rootContext()->setContextProperty("FocusController", m_focusController);

    m_installController = new InstallController(m_serversController, m_serversModel, m_containersModel, m_protocolsModel, m_clientManagementController, m_appSettingsRepository, m_settings, this);
    m_engine->rootContext()->setContextProperty("InstallController", m_installController);

    connect(m_installController, &InstallController::currentContainerUpdated, m_connectionController,
            &ConnectionController::onCurrentContainerUpdated); // TODO remove this

    connect(m_installController, &InstallController::profileCleared,
            m_protocolsUiController, &ProtocolsUiController::updateProtocols);

    m_importController = new ImportController(m_serversController, m_serversModel, m_containersModel, m_appSettingsRepository, m_settings, this);
    m_engine->rootContext()->setContextProperty("ImportController", m_importController);

    m_exportController = new ExportController(m_serversController, m_serversModel, m_containersModel, m_clientManagementController, m_appSettingsRepository, m_settings, this);
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController);

    m_languageUiController = new LanguageUiController(m_appSettingsRepository, m_languageModel, this);
    m_engine->rootContext()->setContextProperty("LanguageUiController", m_languageUiController);

    m_settingsController = new SettingsController(m_serversModel, m_containersModel, m_languageUiController, m_sitesController, m_appSplitTunnelingController, m_serversRepository, m_appSettingsRepository, this);
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController);

    m_serversUiController = new ServersUiController(m_serversController, m_serversRepository, m_appSettingsRepository, m_serversModel, m_containersModel, m_defaultServerContainersModel, this);
    m_engine->rootContext()->setContextProperty("ServersUiController", m_serversUiController);

    m_sitesUiController = new SitesUiController(m_sitesController, m_vpnConnection.get(), m_sitesModel, this);
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesUiController);

    m_allowedDnsUiController = new AllowedDnsUiController(m_allowedDnsController, m_allowedDnsModel, this);
    m_engine->rootContext()->setContextProperty("AllowedDnsController", m_allowedDnsUiController);

    m_appSplitTunnelingUiController = new AppSplitTunnelingUiController(m_appSplitTunnelingController, m_appSplitTunnelingModel, this);
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingController", m_appSplitTunnelingUiController);

    m_systemController = new SystemController(this);
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController);

    m_apiSettingsController = new ApiSettingsController(m_serversController, m_serversModel, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_appSettingsRepository, this);
    m_engine->rootContext()->setContextProperty("ApiSettingsController", m_apiSettingsController);

    m_servicesCatalogUiController = new ServicesCatalogUiController(m_servicesCatalogController, m_apiServicesModel, this);
    m_engine->rootContext()->setContextProperty("ServicesCatalogUiController", m_servicesCatalogUiController);

    m_apiConfigsController = new ApiConfigsController(m_serversController, m_serversModel, m_apiServicesModel, m_servicesCatalogController, m_subscriptionController, this);
    m_engine->rootContext()->setContextProperty("ApiConfigsController", m_apiConfigsController);

    m_apiPremV1MigrationController = new ApiPremV1MigrationController(m_serversController, m_serversModel, m_appSettingsRepository, this);
    m_engine->rootContext()->setContextProperty("ApiPremV1MigrationController", m_apiPremV1MigrationController);

    m_apiNewsController = new ApiNewsController(m_newsModel, m_appSettingsRepository, m_serversController, this);
    m_engine->rootContext()->setContextProperty("ApiNewsController", m_apiNewsController);
}

void CoreController::initAndroidController()
{
#ifdef Q_OS_ANDROID
    if (!AndroidController::initLogging()) {
        qFatal("Android logging initialization failed");
    }
    AndroidController::instance()->setSaveLogs(m_appSettingsRepository->isSaveLogs());
    connect(m_appSettingsRepository, &QAppSettingsRepository::saveLogsChanged, AndroidController::instance(), &AndroidController::setSaveLogs);

    AndroidController::instance()->setScreenshotsEnabled(m_appSettingsRepository->isScreenshotsEnabled());
    connect(m_appSettingsRepository, &QAppSettingsRepository::screenshotsEnabledChanged, AndroidController::instance(), &AndroidController::setScreenshotsEnabled);

    connect(m_serversRepository, &QServersRepository::serverRemoved, AndroidController::instance(), &AndroidController::resetLastServer);

    connect(m_appSettingsRepository, &QAppSettingsRepository::settingsCleared, []() { AndroidController::instance()->resetLastServer(-1); });

    connect(AndroidController::instance(), &AndroidController::initConnectionState, this, [this](Vpn::ConnectionState state) {
        m_connectionController->onConnectionStateChanged(state);
        if (m_vpnConnection)
            m_vpnConnection->restoreConnection();
    });
    if (!AndroidController::instance()->initialize()) {
        qFatal("Android controller initialization failed");
    }

    connect(AndroidController::instance(), &AndroidController::importConfigFromOutside, this, [this](QString data) {
        emit m_pageController->goToPageHome();
        m_importController->extractConfigFromData(data);
        data.clear();
        emit m_pageController->goToPageViewConfig();
    });

    m_engine->addImageProvider(QLatin1String("installedAppImage"), new InstalledAppsImageProvider);
#endif
}

void CoreController::initAppleController()
{
#ifdef Q_OS_IOS
    IosController::Instance()->initialize();
    connect(IosController::Instance(), &IosController::importConfigFromOutside, this, [this](QString data) {
        emit m_pageController->goToPageHome();
        m_importController->extractConfigFromData(data);
        emit m_pageController->goToPageViewConfig();
    });

    connect(IosController::Instance(), &IosController::importBackupFromOutside, this, [this](QString filePath) {
        emit m_pageController->goToPageHome();
        m_pageController->goToPageSettingsBackup();
        emit m_settingsController->importBackupFromOutside(filePath);
    });

    QTimer::singleShot(0, this, [this]() { AmneziaVPN::toggleScreenshots(m_appSettingsRepository->isScreenshotsEnabled()); });

    connect(m_appSettingsRepository, &QAppSettingsRepository::screenshotsEnabledChanged, [](bool enabled) { AmneziaVPN::toggleScreenshots(enabled); });
#endif
}

void CoreController::initSignalHandlers()
{
    initErrorMessagesHandler();

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
    initImportPremiumV2VpnKeyHandler();
    initShowMigrationDrawerHandler();
    initStrictKillSwitchHandler();
}

void CoreController::initNotificationHandler()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    m_notificationHandler = NotificationHandler::create(this);

    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, m_notificationHandler,
            &NotificationHandler::setConnectionState);

    connect(m_notificationHandler, &NotificationHandler::raiseRequested, m_pageController, &PageController::raiseMainWindow);
    connect(m_notificationHandler, &NotificationHandler::connectRequested, m_connectionController,
            static_cast<void (ConnectionController::*)()>(&ConnectionController::openConnection));
    connect(m_notificationHandler, &NotificationHandler::disconnectRequested, m_connectionController,
            &ConnectionController::closeConnection);
    connect(this, &CoreController::translationsUpdated, m_notificationHandler, &NotificationHandler::onTranslationsUpdated);

    auto* trayHandler = qobject_cast<SystemTrayNotificationHandler*>(m_notificationHandler);
    connect(this, &CoreController::websiteUrlChanged, trayHandler, &SystemTrayNotificationHandler::updateWebsiteUrl);
#endif    
}

void CoreController::updateTranslator(const QLocale &locale)
{
    if (!m_translator->isEmpty()) {
        QCoreApplication::removeTranslator(m_translator);
    }

    QStringList availableTranslations;
    QDirIterator it(":/translations", QStringList("amneziavpn_*.qm"), QDir::Files);
    while (it.hasNext()) {
        availableTranslations << it.next();
    }

    // This code allow to load translation for the language only, without country code
    const QString lang = locale.name().split("_").first();
    const QString translationFilePrefix = QString(":/translations/amneziavpn_") + lang;
    QString strFileName = QString(":/translations/amneziavpn_%1.qm").arg(locale.name());
    for (const QString &translation : availableTranslations) {
        if (translation.contains(translationFilePrefix)) {
            strFileName = translation;
            break;
        }
    }

    if (m_translator->load(strFileName)) {
        QCoreApplication::installTranslator(m_translator);
    } else {
        if (m_translator->load(QString(":/translations/amneziavpn_en.qm"))) {
            QCoreApplication::installTranslator(m_translator);
        }
    }

    m_engine->retranslate();

    emit translationsUpdated();
    emit websiteUrlChanged(m_languageUiController->getCurrentSiteUrl());
}

void CoreController::initErrorMessagesHandler()
{
    connect(m_connectionController, &ConnectionController::connectionErrorOccurred, this, [this](ErrorCode errorCode) {
        emit m_pageController->showErrorMessage(errorCode);
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
    });

    connect(m_apiConfigsController, &ApiConfigsController::errorOccurred, m_pageController,
            qOverload<ErrorCode>(&PageController::showErrorMessage));
}

void CoreController::setQmlRoot()
{
    m_systemController->setQmlRoot(m_engine->rootObjects().value(0));
}

void CoreController::initApiCountryModelUpdateHandler()
{
    connect(m_serversModel, &ServersModel::updateApiCountryModel, this, [this]() {
        m_apiCountryModel->updateModel(m_serversModel->getProcessedServerData("apiAvailableCountries").toJsonArray(),
                                       m_serversModel->getProcessedServerData("apiServerCountryCode").toString());
    });
}

void CoreController::initContainerModelUpdateHandler()
{
    connect(m_serversController, &ServersController::gatewayStacksExpanded, this, [this]() {
        m_apiNewsController->fetchNews(false);
    });
}

void CoreController::initAdminConfigRevokedHandler()
{
    // Admin config revocation is now handled by InstallController::clearCachedProfile
}

void CoreController::initPassphraseRequestHandler()
{
    connect(m_installController, &InstallController::passphraseRequestStarted, m_pageController,
            &PageController::showPassphraseRequestDrawer);
    connect(m_pageController, &PageController::passphraseRequestDrawerClosed, m_installController,
            &InstallController::setEncryptedPassphrase);
}

void CoreController::initTranslationsUpdatedHandler()
{
    connect(m_languageUiController, &LanguageUiController::updateTranslations, this, &CoreController::updateTranslator);
    connect(this, &CoreController::translationsUpdated, m_languageUiController, &LanguageUiController::translationsUpdated);
    connect(this, &CoreController::translationsUpdated, m_connectionController, &ConnectionController::onTranslationsUpdated);
}

void CoreController::initLanguageHandler()
{
    connect(m_appSettingsRepository, &QAppSettingsRepository::appLanguageChanged, m_languageUiController, &LanguageUiController::onAppLanguageChanged);
    connect(m_settingsController, &SettingsController::resetLanguageToSystem, m_languageUiController, [this]() {
        m_languageUiController->changeLanguage(m_languageUiController->getSystemLanguageEnum());
    });
}

void CoreController::initAutoConnectHandler()
{
    if (m_settingsController->isAutoConnectEnabled() && m_serversController->getDefaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this]() { m_connectionController->openConnection(); });
    }
}

void CoreController::initAmneziaDnsToggledHandler()
{
    connect(m_appSettingsRepository, &QAppSettingsRepository::useAmneziaDnsChanged, m_serversUiController, [this](bool enabled) {
        Q_UNUSED(enabled);
        m_serversUiController->updateModel();
    });
}

void CoreController::initServersModelUpdateHandler()
{
    connect(m_serversRepository, &QServersRepository::serverAdded,
            m_serversUiController, &ServersUiController::onAddServer);
    connect(m_serversRepository, &QServersRepository::serverEdited,
            m_serversUiController, &ServersUiController::onServerEdited);
    connect(m_serversRepository, &QServersRepository::serverRemoved,
            m_serversUiController, &ServersUiController::onServerRemoved);
    connect(m_serversRepository, &QServersRepository::defaultServerChanged,
            m_serversUiController, &ServersUiController::onDefaultServerChanged);
}

void CoreController::initClientManagementModelUpdateHandler()
{
    connect(m_clientManagementController, &ClientManagementController::clientsUpdated,
            m_clientManagementModel, &ClientManagementModel::updateModel);
}

void CoreController::initSitesModelUpdateHandler()
{
    connect(m_appSettingsRepository, &QAppSettingsRepository::sitesChanged, m_sitesUiController, [this](amnezia::RouteMode mode) {
        Q_UNUSED(mode);
        m_sitesUiController->updateModel();
    });
    connect(m_appSettingsRepository, &QAppSettingsRepository::sitesSplitTunnelingEnabledChanged, m_sitesUiController, [this](bool enabled) {
        Q_UNUSED(enabled);
        m_sitesUiController->updateModel();
    });
    connect(m_appSettingsRepository, &QAppSettingsRepository::routeModeChanged, m_sitesUiController, [this](amnezia::RouteMode mode) {
        Q_UNUSED(mode);
        m_sitesUiController->updateModel();
    });
}

void CoreController::initAllowedDnsModelUpdateHandler()
{
    connect(m_appSettingsRepository, &QAppSettingsRepository::allowedDnsServersChanged, m_allowedDnsUiController, [this](const QStringList &servers) {
        Q_UNUSED(servers);
        m_allowedDnsUiController->updateModel();
    });
}

void CoreController::initAppSplitTunnelingModelUpdateHandler()
{
    connect(m_appSettingsRepository, &QAppSettingsRepository::appsChanged, m_appSplitTunnelingUiController, [this](amnezia::AppsRouteMode mode) {
        Q_UNUSED(mode);
        m_appSplitTunnelingUiController->updateModel();
    });
    connect(m_appSettingsRepository, &QAppSettingsRepository::appsSplitTunnelingEnabledChanged, m_appSplitTunnelingUiController, [this](bool enabled) {
        Q_UNUSED(enabled);
        m_appSplitTunnelingUiController->updateModel();
    });
    connect(m_appSettingsRepository, &QAppSettingsRepository::appsRouteModeChanged, m_appSplitTunnelingUiController, [this](amnezia::AppsRouteMode mode) {
        Q_UNUSED(mode);
        m_appSplitTunnelingUiController->updateModel();
    });
}

void CoreController::initPrepareConfigHandler()
{
    connect(m_connectionController, &ConnectionController::prepareConfig, this, [this]() {
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);

        if (!m_apiConfigsController->isConfigValid()) {
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        if (!m_installController->isConfigValid()) {
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        m_connectionController->openConnection();
    });
}

void CoreController::initImportPremiumV2VpnKeyHandler()
{
    connect(m_apiPremV1MigrationController, &ApiPremV1MigrationController::importPremiumV2VpnKey, this, [this](const QString &vpnKey) {
        m_importController->extractConfigFromData(vpnKey);
        m_importController->importConfig();

        emit m_apiPremV1MigrationController->migrationFinished();
    });
}

void CoreController::initShowMigrationDrawerHandler()
{
    QTimer::singleShot(1000, this, [this]() {
        if (m_apiPremV1MigrationController->isPremV1MigrationReminderActive() && m_apiPremV1MigrationController->hasConfigsToMigration()) {
            m_apiPremV1MigrationController->showMigrationDrawer();
        }
    });
}

void CoreController::initStrictKillSwitchHandler()
{
    connect(m_settingsController, &SettingsController::strictKillSwitchEnabledChanged, m_vpnConnection.get(),
            &VpnConnection::onKillSwitchModeChanged);
}

PageController* CoreController::pageController() const
{
    return m_pageController;
}
