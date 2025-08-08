#include "coreController.h"

#include <QDirIterator>
#include <QTranslator>

#include "core/models/clientInfo.h"

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
    initCoreControllers();
    initModels();
    initUIControllers();
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

    m_openVpnConfigModel = QSharedPointer<OpenVpnConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("OpenVpnConfigModel", m_openVpnConfigModel.get());

    m_shadowSocksConfigModel = QSharedPointer<ShadowSocksConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("ShadowSocksConfigModel", m_shadowSocksConfigModel.get());

    m_cloakConfigModel = QSharedPointer<CloakConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("CloakConfigModel", m_cloakConfigModel.get());

    m_wireGuardConfigModel = QSharedPointer<WireGuardConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("WireGuardConfigModel", m_wireGuardConfigModel.get());

    m_awgConfigModel = QSharedPointer<AwgConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("AwgConfigModel", m_awgConfigModel.get());

    m_xrayConfigModel = QSharedPointer<XrayConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("XrayConfigModel", m_xrayConfigModel.get());

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel = QSharedPointer<Ikev2ConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel.get());
#endif

    m_sftpConfigModel = QSharedPointer<SftpConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("SftpConfigModel", m_sftpConfigModel.get());

    m_socks5ConfigModel = QSharedPointer<Socks5ProxyConfigModel>::create(this);
    m_engine->rootContext()->setContextProperty("Socks5ProxyConfigModel", m_socks5ConfigModel.get());

    m_protocolsModel.reset(new ProtocolsModel(m_openVpnConfigModel, m_shadowSocksConfigModel, m_cloakConfigModel, m_wireGuardConfigModel,
                                              m_awgConfigModel, m_xrayConfigModel,
#ifdef Q_OS_WINDOWS
                                              m_ikev2ConfigModel,
#endif
                                              m_sftpConfigModel, m_socks5ConfigModel, this));
    m_engine->rootContext()->setContextProperty("ProtocolsModel", m_protocolsModel.get());

    auto clientManagementController = QSharedPointer<ClientManagementController>::create(m_settings, this);
    m_clientManagementModel.reset(new ClientManagementModel(clientManagementController, this));

    m_clientManagementUIController.reset(new ClientManagementUIController(clientManagementController, this));
    m_engine->rootContext()->setContextProperty("ClientManagementUIController", m_clientManagementUIController.get());
    m_engine->rootContext()->setContextProperty("ClientManagementModel", m_clientManagementModel.get());

    m_apiServicesModel.reset(new ApiServicesModel(this));
    m_engine->rootContext()->setContextProperty("ApiServicesModel", m_apiServicesModel.get());

    m_apiCountryModel.reset(new ApiCountryModel(this));
    m_engine->rootContext()->setContextProperty("ApiCountryModel", m_apiCountryModel.get());

    m_apiAccountInfoModel.reset(new ApiAccountInfoModel(this));
    m_engine->rootContext()->setContextProperty("ApiAccountInfoModel", m_apiAccountInfoModel.get());

    m_apiDevicesModel.reset(new ApiDevicesModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ApiDevicesModel", m_apiDevicesModel.get());

    m_sitesModel.reset(new SitesModel(m_splitTunnelingController, this));
    m_engine->rootContext()->setContextProperty("SitesModel", m_sitesModel.get());

    m_allowedDnsModel.reset(new AllowedDnsModel(m_dnsController, this));
    m_engine->rootContext()->setContextProperty("AllowedDnsModel", m_allowedDnsModel.get());

    m_appSplitTunnelingModel.reset(new AppSplitTunnelingModel(m_splitTunnelingController, this));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingModel", m_appSplitTunnelingModel.get());

    m_languageModel.reset(new LanguageModel(m_settingsController, this));
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel.get());
}

void CoreController::initCoreControllers()
{
    m_settingsController = QSharedPointer<SettingsController>::create(m_settings, this);
    m_dnsController = QSharedPointer<DnsController>::create(m_settings, this);
    m_splitTunnelingController = QSharedPointer<SplitTunnelingController>::create(m_settings, m_vpnConnection, this);
    m_exportController = QSharedPointer<ExportController>::create(m_settings, this);
    m_installController = QSharedPointer<InstallController>::create(m_settings, this);
}

void CoreController::initUIControllers()
{
    auto coreConnectionController = QSharedPointer<ConnectionController>::create(m_vpnConnection, m_settings, this);
    m_connectionController.reset(
            new ConnectionUIController(m_serversModel, m_containersModel, m_clientManagementModel, coreConnectionController, m_settings));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    m_pageController.reset(new PageController(m_serversModel, m_settings));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_focusController.reset(new FocusController(m_engine, this));
    m_engine->rootContext()->setContextProperty("FocusController", m_focusController.get());

    auto clientManagementController = m_clientManagementUIController->getClientManagementController();
    m_exportUIController.reset(new ExportUIController(m_serversModel, m_containersModel, m_clientManagementModel, m_exportController, clientManagementController));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportUIController.get());

    m_installUIController.reset(new InstallUIController(m_serversModel, m_containersModel, m_protocolsModel, m_clientManagementModel, m_installController, clientManagementController));
    m_engine->rootContext()->setContextProperty("InstallController", m_installUIController.get());

    connect(m_installUIController.get(), &InstallUIController::currentContainerUpdated, m_connectionController.get(),
            &ConnectionUIController::onCurrentContainerUpdated);

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_settingsUIController.reset(
            new SettingsUIController(m_serversModel, m_containersModel, m_languageModel, m_sitesModel, m_appSplitTunnelingModel, m_settingsController));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsUIController.get());

    m_siteSplitUIController.reset(new SiteSplitUIController(m_splitTunnelingController, m_sitesModel));
    m_engine->rootContext()->setContextProperty("SitesController", m_siteSplitUIController.get());

    m_allowedDnsUIController.reset(new AllowedDnsUIController(m_dnsController, m_allowedDnsModel));
    m_engine->rootContext()->setContextProperty("AllowedDnsController", m_allowedDnsUIController.get());

    m_appSplitUIController.reset(new AppSplitUIController(m_splitTunnelingController, m_appSplitTunnelingModel));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingController", m_appSplitUIController.get());

    m_systemController.reset(new SystemController(m_settings));
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController.get());

    m_apiSettingsCoreController = QSharedPointer<ApiSettingsController>::create(m_serversModel, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_settings);
    m_apiConfigsCoreController = QSharedPointer<ApiConfigsController>::create(m_serversModel, m_apiServicesModel, m_settings);
    m_apiPremV1MigrationCoreController = QSharedPointer<ApiPremV1MigrationController>::create(m_serversModel, m_settings, this);

    m_apiSettingsUIController.reset(new ApiSettingsUIController(m_serversModel, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_apiSettingsCoreController, m_settings));
    m_engine->rootContext()->setContextProperty("ApiSettingsController", m_apiSettingsUIController.get());

    m_apiConfigUIController.reset(new ApiConfigUIController(m_serversModel, m_apiServicesModel, m_apiConfigsCoreController, m_settings));
    m_engine->rootContext()->setContextProperty("ApiConfigsController", m_apiConfigUIController.get());

    m_apiPremV1MigrationUIController.reset(new ApiPremV1MigrationUIController(m_serversModel, m_apiPremV1MigrationCoreController, m_settings));
    m_engine->rootContext()->setContextProperty("ApiPremV1MigrationController", m_apiPremV1MigrationUIController.get());
    
    setupControllerSignalConnections();
}

void CoreController::setupControllerSignalConnections()
{
    auto clientManagementController = m_clientManagementUIController->getClientManagementController();
    
    connect(m_exportController.data(), &ExportController::clientAppendRequested,
            clientManagementController.data(), 
            [clientManagementController](const DockerContainer container, const ServerCredentials &credentials, 
                                       const ContainerConfig &containerConfig, const QString &clientName, 
                                       const QSharedPointer<ServerController> &serverController) {
                QList<ClientInfo> clientsList;
                ErrorCode result = clientManagementController->appendClient(container, credentials, containerConfig, 
                                                                           clientName, serverController, clientsList);
                emit clientManagementController->clientAppendCompleted(result);
            });
    
    connect(m_exportController.data(), &ExportController::nativeConfigClientAppendRequested,
            clientManagementController.data(),
            [clientManagementController](const QSharedPointer<ProtocolConfig> &protocolConfig, const QString &clientName, 
                                       const DockerContainer container, const ServerCredentials &credentials, 
                                       const QSharedPointer<ServerController> &serverController) {
                QList<ClientInfo> clientsList;
                ErrorCode result = clientManagementController->appendClient(protocolConfig, clientName, container, 
                                                                           credentials, serverController, clientsList);
                emit clientManagementController->nativeConfigClientAppendCompleted(result);
            });
    
    connect(clientManagementController.data(), &ClientManagementController::clientAppendCompleted,
            m_exportController.data(), &ExportController::onClientAppendCompleted);
    
    connect(clientManagementController.data(), &ClientManagementController::nativeConfigClientAppendCompleted,
            m_exportController.data(), &ExportController::onNativeConfigClientAppendCompleted);
            
    connect(m_installController.data(), &InstallController::clientAppendRequested,
            clientManagementController.data(),
            [clientManagementController](const DockerContainer container, const ServerCredentials &credentials,
                                       const ContainerConfig &containerConfig, const QString &clientName,
                                       const QSharedPointer<ServerController> &serverController) {
                QList<ClientInfo> clientsList;
                clientManagementController->appendClient(container, credentials, containerConfig,
                                                        clientName, serverController, clientsList);
            });
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
        emit m_settingsUIController->importBackupFromOutside(filePath);
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
            static_cast<void (ConnectionUIController::*)()>(&ConnectionUIController::openConnection));
    connect(m_notificationHandler.get(), &NotificationHandler::disconnectRequested, m_connectionController.get(),
            &ConnectionUIController::closeConnection);
    connect(this, &CoreController::translationsUpdated, m_notificationHandler.get(), &NotificationHandler::onTranslationsUpdated);
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
}

void CoreController::initErrorMessagesHandler()
{
    connect(m_connectionController.get(), &ConnectionUIController::connectionErrorOccurred, this, [this](ErrorCode errorCode) {
        emit m_pageController->showErrorMessage(errorCode);
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
    });

    connect(m_apiConfigUIController.get(), &ApiConfigUIController::errorOccurred, m_pageController.get(),
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
    connect(this, &CoreController::translationsUpdated, m_connectionController.get(), &ConnectionUIController::onTranslationsUpdated);
}

void CoreController::initAutoConnectHandler()
{
    if (m_settingsUIController->isAutoConnectEnabled() && m_serversModel->getDefaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this]() { m_connectionController->openConnection(); });
    }
}

void CoreController::initAmneziaDnsToggledHandler()
{
    connect(m_settingsUIController.get(), &SettingsUIController::amneziaDnsToggled, m_serversModel.get(), &ServersModel::toggleAmneziaDns);
}

void CoreController::initPrepareConfigHandler()
{
    connect(m_connectionController.get(), &ConnectionUIController::prepareConfig, this, [this]() {
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);

        if (!m_apiConfigUIController->isConfigValid()) {
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
    connect(m_apiPremV1MigrationUIController.get(), &ApiPremV1MigrationUIController::importPremiumV2VpnKey, this, [this](const QString &vpnKey) {
        m_importController->extractConfigFromData(vpnKey);
        m_importController->importConfig();

        emit m_apiPremV1MigrationUIController->migrationFinished();
    });
}

void CoreController::initShowMigrationDrawerHandler()
{
    QTimer::singleShot(1000, this, [this]() {
        if (m_apiPremV1MigrationUIController->isPremV1MigrationReminderActive() && m_apiPremV1MigrationUIController->hasConfigsToMigration()) {
            m_apiPremV1MigrationUIController->showMigrationDrawer();
        }
    });
}

void CoreController::initStrictKillSwitchHandler()
{
    connect(m_settingsUIController.get(), &SettingsUIController::strictKillSwitchEnabledChanged, m_vpnConnection.get(),
            &VpnConnection::onKillSwitchModeChanged);
}

QSharedPointer<PageController> CoreController::pageController() const
{
    return m_pageController;
}
