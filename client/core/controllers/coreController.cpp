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
    initModels();
    initControllers();
    initSignalHandlers();

    initAndroidController();
    initAppleController();

    initNotificationHandler();

    auto locale = m_settings->getAppLanguage();
    m_translator.reset(new QTranslator());
    updateTranslator(locale);
}

void CoreController::initModels()
{
    m_containersModel.reset(new ContainersModel(this));
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel.get());

    m_defaultServerContainersModel.reset(new ContainersModel(this));
    m_engine->rootContext()->setContextProperty("DefaultServerContainersModel", m_defaultServerContainersModel.get());

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());

    m_languageModel.reset(new LanguageModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel.get());

    m_sitesModel.reset(new SitesModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("SitesModel", m_sitesModel.get());

    m_allowedDnsModel.reset(new AllowedDnsModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("AllowedDnsModel", m_allowedDnsModel.get());

    m_appSplitTunnelingModel.reset(new AppSplitTunnelingModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingModel", m_appSplitTunnelingModel.get());

    m_protocolsModel.reset(new ProtocolsModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ProtocolsModel", m_protocolsModel.get());

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

    m_clientManagementModel.reset(new ClientManagementModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ClientManagementModel", m_clientManagementModel.get());

    m_apiServicesModel.reset(new ApiServicesModel(this));
    m_engine->rootContext()->setContextProperty("ApiServicesModel", m_apiServicesModel.get());

    m_apiCountryModel.reset(new ApiCountryModel(this));
    m_engine->rootContext()->setContextProperty("ApiCountryModel", m_apiCountryModel.get());

    m_apiAccountInfoModel.reset(new ApiAccountInfoModel(this));
    m_engine->rootContext()->setContextProperty("ApiAccountInfoModel", m_apiAccountInfoModel.get());

    m_apiDevicesModel.reset(new ApiDevicesModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ApiDevicesModel", m_apiDevicesModel.get());
}

void CoreController::initControllers()
{
    m_connectionController.reset(
            new ConnectionController(m_serversModel, m_containersModel, m_clientManagementModel, m_vpnConnection, m_settings));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    m_pageController.reset(new PageController(m_serversModel, m_settings));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_focusController.reset(new FocusController(m_engine, this));
    m_engine->rootContext()->setContextProperty("FocusController", m_focusController.get());

    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_protocolsModel, m_clientManagementModel, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());

    connect(m_installController.get(), &InstallController::currentContainerUpdated, m_connectionController.get(),
            &ConnectionController::onCurrentContainerUpdated); // TODO remove this

    connect(m_installController.get(), &InstallController::profileCleared,
            m_protocolsModel.get(), &ProtocolsModel::updateModel);

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(new ExportController(m_serversModel, m_containersModel, m_clientManagementModel, m_settings));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_settingsController.reset(
            new SettingsController(m_serversModel, m_containersModel, m_languageModel, m_sitesModel, m_appSplitTunnelingModel, m_settings));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());

    m_sitesController.reset(new SitesController(m_settings, m_vpnConnection, m_sitesModel));
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesController.get());

    m_allowedDnsController.reset(new AllowedDnsController(m_settings, m_allowedDnsModel));
    m_engine->rootContext()->setContextProperty("AllowedDnsController", m_allowedDnsController.get());

    m_appSplitTunnelingController.reset(new AppSplitTunnelingController(m_settings, m_appSplitTunnelingModel));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingController", m_appSplitTunnelingController.get());

    m_systemController.reset(new SystemController(m_settings));
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController.get());

    m_apiSettingsController.reset(
            new ApiSettingsController(m_serversModel, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_settings));
    m_engine->rootContext()->setContextProperty("ApiSettingsController", m_apiSettingsController.get());

    m_apiConfigsController.reset(new ApiConfigsController(m_serversModel, m_apiServicesModel, m_settings));
    m_engine->rootContext()->setContextProperty("ApiConfigsController", m_apiConfigsController.get());

    m_apiPremV1MigrationController.reset(new ApiPremV1MigrationController(m_serversModel, m_settings, this));
    m_engine->rootContext()->setContextProperty("ApiPremV1MigrationController", m_apiPremV1MigrationController.get());
}

void CoreController::initAndroidController()
{
#ifdef Q_OS_ANDROID
    if (!AndroidController::initLogging()) {
        qFatal("Android logging initialization failed");
    }
    AndroidController::instance()->setSaveLogs(m_settings->isSaveLogs());
    connect(m_settings.get(), &Settings::saveLogsChanged, AndroidController::instance(), &AndroidController::setSaveLogs);

    AndroidController::instance()->setScreenshotsEnabled(m_settings->isScreenshotsEnabled());
    connect(m_settings.get(), &Settings::screenshotsEnabledChanged, AndroidController::instance(), &AndroidController::setScreenshotsEnabled);

    connect(m_settings.get(), &Settings::serverRemoved, AndroidController::instance(), &AndroidController::resetLastServer);

    connect(m_settings.get(), &Settings::settingsCleared, []() { AndroidController::instance()->resetLastServer(-1); });

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

    QTimer::singleShot(0, this, [this]() { AmneziaVPN::toggleScreenshots(m_settings->isScreenshotsEnabled()); });

    connect(m_settings.get(), &Settings::screenshotsEnabledChanged, [](bool enabled) { AmneziaVPN::toggleScreenshots(enabled); });
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
    initAutoConnectHandler();
    initAmneziaDnsToggledHandler();
    initPrepareConfigHandler();
    initImportPremiumV2VpnKeyHandler();
    initShowMigrationDrawerHandler();
    initStrictKillSwitchHandler();
}

void CoreController::initNotificationHandler()
{
#ifndef Q_OS_ANDROID
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
            m_settings->setAppLanguage(locale);
        }
    } else {
        m_settings->setAppLanguage(QLocale::English);
    }

    m_engine->retranslate();

    emit translationsUpdated();
    emit websiteUrlChanged(m_languageModel->getCurrentSiteUrl());
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
    connect(m_serversModel.get(), &ServersModel::containersUpdated, m_containersModel.get(), &ContainersModel::updateModel);
    connect(m_serversModel.get(), &ServersModel::defaultServerContainersUpdated, m_defaultServerContainersModel.get(),
            &ContainersModel::updateModel);
    m_serversModel->resetModel();
}

void CoreController::initAdminConfigRevokedHandler()
{
    connect(m_clientManagementModel.get(), &ClientManagementModel::adminConfigRevoked, m_serversModel.get(),
            &ServersModel::clearCachedProfile);
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
    connect(m_languageModel.get(), &LanguageModel::updateTranslations, this, &CoreController::updateTranslator);
    connect(this, &CoreController::translationsUpdated, m_languageModel.get(), &LanguageModel::translationsUpdated);
    connect(this, &CoreController::translationsUpdated, m_connectionController.get(), &ConnectionController::onTranslationsUpdated);
}

void CoreController::initAutoConnectHandler()
{
    if (m_settingsController->isAutoConnectEnabled() && m_serversModel->getDefaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this]() { m_connectionController->openConnection(); });
    }
}

void CoreController::initAmneziaDnsToggledHandler()
{
    connect(m_settingsController.get(), &SettingsController::amneziaDnsToggled, m_serversModel.get(), &ServersModel::toggleAmneziaDns);
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
