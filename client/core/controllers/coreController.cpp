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

    m_translator.reset(new QTranslator());
    updateTranslator(m_appSettingsRepository->getAppLanguage());
}

void CoreController::initModels()
{
    m_containersModel.reset(new ContainersModel(this));
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel.get());

    m_defaultServerContainersModel.reset(new ContainersModel(this));
    m_engine->rootContext()->setContextProperty("DefaultServerContainersModel", m_defaultServerContainersModel.get());

    m_serversModel.reset(new ServersModel(this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());

    m_languageModel.reset(new LanguageModel(this));
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel.get());

    m_sitesModel.reset(new SitesModel(this));
    m_engine->rootContext()->setContextProperty("SitesModel", m_sitesModel.get());

    m_allowedDnsModel.reset(new AllowedDnsModel(this));
    m_engine->rootContext()->setContextProperty("AllowedDnsModel", m_allowedDnsModel.get());

    m_appSplitTunnelingModel.reset(new AppSplitTunnelingModel(this));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingModel", m_appSplitTunnelingModel.get());

    m_protocolsModel.reset(new ProtocolsModel(this));
    m_engine->rootContext()->setContextProperty("ProtocolsModel", m_protocolsModel.get());
    
    m_protocolsUiController.reset(new ProtocolsUiController(m_protocolsModel, this));
    m_engine->rootContext()->setContextProperty("ProtocolsUiController", m_protocolsUiController.get());

    m_openVpnConfigModel.reset(new OpenVpnConfigModel(this));
    m_engine->rootContext()->setContextProperty("OpenVpnConfigModel", m_openVpnConfigModel.get());

    m_shadowSocksConfigModel.reset(new ShadowSocksConfigModel(this));
    m_engine->rootContext()->setContextProperty("ShadowSocksConfigModel", m_shadowSocksConfigModel.get());

    m_cloakConfigModel.reset(new CloakConfigModel(this));
    m_engine->rootContext()->setContextProperty("CloakConfigModel", m_cloakConfigModel.get());

    m_wireGuardConfigModel.reset(new WireGuardConfigModel(this));
    m_engine->rootContext()->setContextProperty("WireGuardConfigModel", m_wireGuardConfigModel.get());

    m_awgConfigModel.reset(new AwgConfigModel(this));
    m_engine->rootContext()->setContextProperty("AwgConfigModel", m_awgConfigModel.get());

    m_xrayConfigModel.reset(new XrayConfigModel(this));
    m_engine->rootContext()->setContextProperty("XrayConfigModel", m_xrayConfigModel.get());

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel.reset(new Ikev2ConfigModel(this));
    m_engine->rootContext()->setContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel.get());
#endif

    m_sftpConfigModel.reset(new SftpConfigModel(this));
    m_engine->rootContext()->setContextProperty("SftpConfigModel", m_sftpConfigModel.get());

    m_socks5ConfigModel.reset(new Socks5ProxyConfigModel(this));
    m_engine->rootContext()->setContextProperty("Socks5ProxyConfigModel", m_socks5ConfigModel.get());

    m_clientManagementModel.reset(new ClientManagementModel(this));
    m_engine->rootContext()->setContextProperty("ClientManagementModel", m_clientManagementModel.get());

    m_apiServicesModel.reset(new ApiServicesModel(this));
    m_engine->rootContext()->setContextProperty("ApiServicesModel", m_apiServicesModel.get());

    m_apiCountryModel.reset(new ApiCountryModel(this));
    m_engine->rootContext()->setContextProperty("ApiCountryModel", m_apiCountryModel.get());

    m_apiAccountInfoModel.reset(new ApiAccountInfoModel(this));
    m_engine->rootContext()->setContextProperty("ApiAccountInfoModel", m_apiAccountInfoModel.get());

    m_apiDevicesModel.reset(new ApiDevicesModel(this));
    m_engine->rootContext()->setContextProperty("ApiDevicesModel", m_apiDevicesModel.get());

    m_newsModel.reset(new NewsModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("NewsModel", m_newsModel.get());
}

void CoreController::initRepositories()
{
    m_serversRepository = QSharedPointer<QServersRepository>::create(m_settings, this);
    m_appSettingsRepository = QSharedPointer<QAppSettingsRepository>::create(m_settings, this);
}

void CoreController::initCoreControllers()
{
    std::shared_ptr<ServersRepository> serversRepo(m_serversRepository.get(), [](ServersRepository*){});
    std::shared_ptr<AppSettingsRepository> appSettingsRepo(m_appSettingsRepository.get(), [](AppSettingsRepository*){});
    
    m_serversController = QSharedPointer<ServersController>::create(serversRepo, appSettingsRepo, this);
    m_appSplitTunnelingController = QSharedPointer<AppSplitTunnelingController>::create(appSettingsRepo);
    m_clientManagementController = QSharedPointer<ClientManagementController>::create(serversRepo, this);
    m_sitesController = QSharedPointer<SitesController>::create(appSettingsRepo);
    m_allowedDnsController = QSharedPointer<AllowedDnsController>::create(appSettingsRepo);
    m_servicesCatalogController = QSharedPointer<ServicesCatalogController>::create(appSettingsRepo);
    m_subscriptionController = QSharedPointer<SubscriptionController>::create(serversRepo, appSettingsRepo);
}

void CoreController::initControllers()
{
    m_connectionController.reset(
            new ConnectionController(m_serversController, m_serversModel, m_containersModel, m_clientManagementModel, m_vpnConnection, m_appSettingsRepository, m_settings));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    m_pageController.reset(new PageController(m_serversModel, m_appSettingsRepository));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_focusController.reset(new FocusController(m_engine, this));
    m_engine->rootContext()->setContextProperty("FocusController", m_focusController.get());

    m_installController.reset(new InstallController(m_serversController, m_serversModel, m_containersModel, m_protocolsModel, m_clientManagementController, m_appSettingsRepository, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());

    connect(m_installController.get(), &InstallController::currentContainerUpdated, m_connectionController.get(),
            &ConnectionController::onCurrentContainerUpdated); // TODO remove this

    connect(m_installController.get(), &InstallController::profileCleared,
            m_protocolsUiController.get(), &ProtocolsUiController::updateProtocols);

    m_importController.reset(new ImportController(m_serversController, m_serversModel, m_containersModel, m_appSettingsRepository, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(new ExportController(m_serversController, m_serversModel, m_containersModel, m_clientManagementController, m_appSettingsRepository, m_settings));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_languageUiController = QSharedPointer<LanguageUiController>::create(m_appSettingsRepository, m_languageModel, this);
    m_engine->rootContext()->setContextProperty("LanguageUiController", m_languageUiController.get());

    m_settingsController.reset(
            new SettingsController(m_serversModel, m_containersModel, m_languageUiController, m_sitesController, m_appSplitTunnelingController, m_serversRepository, m_appSettingsRepository));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());

    m_serversUiController = QSharedPointer<ServersUiController>::create(m_serversController, m_serversRepository, m_appSettingsRepository, m_serversModel, m_containersModel, m_defaultServerContainersModel);
    m_engine->rootContext()->setContextProperty("ServersUiController", m_serversUiController.get());

    m_sitesUiController.reset(new SitesUiController(m_sitesController, m_vpnConnection, m_sitesModel, this));
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesUiController.get());

    m_allowedDnsUiController.reset(new AllowedDnsUiController(m_allowedDnsController, m_allowedDnsModel, this));
    m_engine->rootContext()->setContextProperty("AllowedDnsController", m_allowedDnsUiController.get());

    m_appSplitTunnelingUiController.reset(new AppSplitTunnelingUiController(m_appSplitTunnelingController, m_appSplitTunnelingModel, this));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingController", m_appSplitTunnelingUiController.get());

    m_systemController.reset(new SystemController());
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController.get());

    m_apiSettingsController.reset(
            new ApiSettingsController(m_serversController, m_serversModel, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_appSettingsRepository));
    m_engine->rootContext()->setContextProperty("ApiSettingsController", m_apiSettingsController.get());

    m_servicesCatalogUiController.reset(new ServicesCatalogUiController(m_servicesCatalogController, m_apiServicesModel, this));
    m_engine->rootContext()->setContextProperty("ServicesCatalogUiController", m_servicesCatalogUiController.get());

    m_apiConfigsController.reset(new ApiConfigsController(m_serversController, m_serversModel, m_apiServicesModel, m_servicesCatalogController, m_subscriptionController));
    m_engine->rootContext()->setContextProperty("ApiConfigsController", m_apiConfigsController.get());

    m_apiPremV1MigrationController.reset(new ApiPremV1MigrationController(m_serversController, m_serversModel, m_appSettingsRepository, this));
    m_engine->rootContext()->setContextProperty("ApiPremV1MigrationController", m_apiPremV1MigrationController.get());

    m_apiNewsController.reset(new ApiNewsController(m_newsModel, m_appSettingsRepository, m_serversController, this));
    m_engine->rootContext()->setContextProperty("ApiNewsController", m_apiNewsController.get());
}

void CoreController::initAndroidController()
{
#ifdef Q_OS_ANDROID
    if (!AndroidController::initLogging()) {
        qFatal("Android logging initialization failed");
    }
    AndroidController::instance()->setSaveLogs(m_appSettingsRepository->isSaveLogs());
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::saveLogsChanged, AndroidController::instance(), &AndroidController::setSaveLogs);

    AndroidController::instance()->setScreenshotsEnabled(m_appSettingsRepository->isScreenshotsEnabled());
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::screenshotsEnabledChanged, AndroidController::instance(), &AndroidController::setScreenshotsEnabled);

    connect(m_serversRepository.get(), &QServersRepository::serverRemoved, AndroidController::instance(), &AndroidController::resetLastServer);

    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::settingsCleared, []() { AndroidController::instance()->resetLastServer(-1); });

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

    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::screenshotsEnabledChanged, [](bool enabled) { AmneziaVPN::toggleScreenshots(enabled); });
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
    m_notificationHandler.reset(NotificationHandler::create(nullptr));

    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, m_notificationHandler.get(),
            &NotificationHandler::setConnectionState);

    connect(m_notificationHandler.get(), &NotificationHandler::raiseRequested, m_pageController.get(), &PageController::raiseMainWindow);
    connect(m_notificationHandler.get(), &NotificationHandler::connectRequested, m_connectionController.get(),
            static_cast<void (ConnectionController::*)()>(&ConnectionController::openConnection));
    connect(m_notificationHandler.get(), &NotificationHandler::disconnectRequested, m_connectionController.get(),
            &ConnectionController::closeConnection);
    connect(this, &CoreController::translationsUpdated, m_notificationHandler.get(), &NotificationHandler::onTranslationsUpdated);

    auto* trayHandler = qobject_cast<SystemTrayNotificationHandler*>(m_notificationHandler.get());
    connect(this, &CoreController::websiteUrlChanged, trayHandler, &SystemTrayNotificationHandler::updateWebsiteUrl);
#endif    
}

void CoreController::updateTranslator(const QLocale &locale)
{
    if (!m_translator->isEmpty()) {
        QCoreApplication::removeTranslator(m_translator.get());
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
        if (QCoreApplication::installTranslator(m_translator.get())) {
            m_appSettingsRepository->setAppLanguage(locale);
        }
    } else {
        m_appSettingsRepository->setAppLanguage(QLocale::English);
    }

    m_engine->retranslate();

    emit translationsUpdated();
    emit websiteUrlChanged(m_languageUiController->getCurrentSiteUrl());
}

void CoreController::initErrorMessagesHandler()
{
    connect(m_connectionController.get(), &ConnectionController::connectionErrorOccurred, this, [this](ErrorCode errorCode) {
        emit m_pageController->showErrorMessage(errorCode);
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
    });

    connect(m_apiConfigsController.get(), &ApiConfigsController::errorOccurred, m_pageController.get(),
            qOverload<ErrorCode>(&PageController::showErrorMessage));
}

void CoreController::setQmlRoot()
{
    m_systemController->setQmlRoot(m_engine->rootObjects().value(0));
}

void CoreController::initApiCountryModelUpdateHandler()
{
    connect(m_serversModel.get(), &ServersModel::updateApiCountryModel, this, [this]() {
        m_apiCountryModel->updateModel(m_serversModel->getProcessedServerData("apiAvailableCountries").toJsonArray(),
                                       m_serversModel->getProcessedServerData("apiServerCountryCode").toString());
    });
}

void CoreController::initContainerModelUpdateHandler()
{
    connect(m_serversController.get(), &ServersController::gatewayStacksExpanded, this, [this]() {
        m_apiNewsController->fetchNews(false);
    });
}

void CoreController::initAdminConfigRevokedHandler()
{
    // Admin config revocation is now handled by InstallController::clearCachedProfile
}

void CoreController::initPassphraseRequestHandler()
{
    connect(m_installController.get(), &InstallController::passphraseRequestStarted, m_pageController.get(),
            &PageController::showPassphraseRequestDrawer);
    connect(m_pageController.get(), &PageController::passphraseRequestDrawerClosed, m_installController.get(),
            &InstallController::setEncryptedPassphrase);
}

void CoreController::initTranslationsUpdatedHandler()
{
    connect(m_languageUiController.get(), &LanguageUiController::updateTranslations, this, &CoreController::updateTranslator);
    connect(this, &CoreController::translationsUpdated, m_languageUiController.get(), &LanguageUiController::translationsUpdated);
    connect(this, &CoreController::translationsUpdated, m_connectionController.get(), &ConnectionController::onTranslationsUpdated);
}

void CoreController::initLanguageHandler()
{
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::appLanguageChanged, m_languageUiController.get(), &LanguageUiController::onAppLanguageChanged);
    connect(m_settingsController.get(), &SettingsController::resetLanguageToSystem, m_languageUiController.get(), [this]() {
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
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::useAmneziaDnsChanged, m_serversUiController.get(), [this](bool enabled) {
        Q_UNUSED(enabled);
        m_serversUiController->updateModel();
    });
}

void CoreController::initServersModelUpdateHandler()
{
    connect(m_serversRepository.get(), &QServersRepository::serverAdded,
            m_serversUiController.get(), &ServersUiController::onAddServer);
    connect(m_serversRepository.get(), &QServersRepository::serverEdited,
            m_serversUiController.get(), &ServersUiController::onServerEdited);
    connect(m_serversRepository.get(), &QServersRepository::serverRemoved,
            m_serversUiController.get(), &ServersUiController::onServerRemoved);
    connect(m_serversRepository.get(), &QServersRepository::defaultServerChanged,
            m_serversUiController.get(), &ServersUiController::onDefaultServerChanged);
}

void CoreController::initClientManagementModelUpdateHandler()
{
    connect(m_clientManagementController.get(), &ClientManagementController::clientsUpdated,
            m_clientManagementModel.get(), &ClientManagementModel::updateModel);
}

void CoreController::initSitesModelUpdateHandler()
{
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::sitesChanged, m_sitesUiController.get(), [this](amnezia::RouteMode mode) {
        Q_UNUSED(mode);
        m_sitesUiController->updateModel();
    });
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::sitesSplitTunnelingEnabledChanged, m_sitesUiController.get(), [this](bool enabled) {
        Q_UNUSED(enabled);
        m_sitesUiController->updateModel();
    });
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::routeModeChanged, m_sitesUiController.get(), [this](amnezia::RouteMode mode) {
        Q_UNUSED(mode);
        m_sitesUiController->updateModel();
    });
}

void CoreController::initAllowedDnsModelUpdateHandler()
{
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::allowedDnsServersChanged, m_allowedDnsUiController.get(), [this](const QStringList &servers) {
        Q_UNUSED(servers);
        m_allowedDnsUiController->updateModel();
    });
}

void CoreController::initAppSplitTunnelingModelUpdateHandler()
{
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::appsChanged, m_appSplitTunnelingUiController.get(), [this](amnezia::AppsRouteMode mode) {
        Q_UNUSED(mode);
        m_appSplitTunnelingUiController->updateModel();
    });
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::appsSplitTunnelingEnabledChanged, m_appSplitTunnelingUiController.get(), [this](bool enabled) {
        Q_UNUSED(enabled);
        m_appSplitTunnelingUiController->updateModel();
    });
    connect(m_appSettingsRepository.get(), &QAppSettingsRepository::appsRouteModeChanged, m_appSplitTunnelingUiController.get(), [this](amnezia::AppsRouteMode mode) {
        Q_UNUSED(mode);
        m_appSplitTunnelingUiController->updateModel();
    });
}

void CoreController::initPrepareConfigHandler()
{
    connect(m_connectionController.get(), &ConnectionController::prepareConfig, this, [this]() {
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
    connect(m_apiPremV1MigrationController.get(), &ApiPremV1MigrationController::importPremiumV2VpnKey, this, [this](const QString &vpnKey) {
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
    connect(m_settingsController.get(), &SettingsController::strictKillSwitchEnabledChanged, m_vpnConnection.get(),
            &VpnConnection::onKillSwitchModeChanged);
}

QSharedPointer<PageController> CoreController::pageController() const
{
    return m_pageController;
}
