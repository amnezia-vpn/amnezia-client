#include "coreController.h"

#include <QDirIterator>
#include <QDateTime>
#include <QSettings>
#include <QTranslator>

#if defined(Q_OS_ANDROID)
    #include "core/installedAppsImageProvider.h"
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/ios_controller.h"
    #include "platforms/ios/fblink_ios_bridge.h"
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

    m_translator.reset(new QTranslator());
    updateTranslator(m_settings->getAppLanguage());
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

    m_newsModel.reset(new NewsModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("NewsModel", m_newsModel.get());
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

    m_apiNewsController.reset(new ApiNewsController(m_newsModel, m_settings, m_serversModel, this));
    m_engine->rootContext()->setContextProperty("ApiNewsController", m_apiNewsController.get());

    m_fbLinkController.reset(new FBLinkController(m_importController.get(), m_settings, m_serversModel.get(), this));
    m_engine->rootContext()->setContextProperty("FBLinkController", m_fbLinkController.get());

    m_updateController.reset(new UpdateController(this));
    m_engine->rootContext()->setContextProperty("UpdateController", m_updateController.get());
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

    QTimer::singleShot(0, this, [this]() { FBLink::toggleScreenshots(m_settings->isScreenshotsEnabled()); });

    connect(m_settings.get(), &Settings::screenshotsEnabledChanged, [](bool enabled) { FBLink::toggleScreenshots(enabled); });
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
    initFBLinkDnsToggledHandler();
    initPrepareConfigHandler();
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
    // We must use "FBLink" casing since our build scripts renamed all files
    QDirIterator it(":/translations", QStringList("FBLink_*.qm"), QDir::Files);
    while (it.hasNext()) {
        availableTranslations << it.next();
    }

    // This code allow to load translation for the language only, without country code
    const QString lang = locale.name().split("_").first();
    const QString translationFilePrefix = QString(":/translations/FBLink_") + lang;
    QString strFileName = QString(":/translations/FBLink_%1.qm").arg(locale.name());
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
    connect(m_serversModel.get(), &ServersModel::gatewayStacksExpanded, this, [this]() {
        if (m_serversModel->hasServersFromGatewayApi()) {
            m_apiNewsController->fetchNews(false);
        }
    });
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
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    const qint64 safeModeUntilEpoch = qSettings.value("Conf/safeModeUntilEpochSec", 0).toLongLong();
    if (safeModeUntilEpoch > QDateTime::currentSecsSinceEpoch()) {
        qWarning() << "Auto-connect is skipped because safe mode is active until" << safeModeUntilEpoch;
        return;
    }

    if (!m_settingsController->isAutoConnectEnabled() || m_serversModel->getDefaultServerIndex() < 0) {
        return;
    }

    auto autoConnect = [this]() {
        m_connectionController->openConnection();
    };

    if (!m_fbLinkController || !m_fbLinkController->isLoggedIn()) {
        QTimer::singleShot(1000, this, autoConnect);
        return;
    }

    auto triggered = QSharedPointer<bool>::create(false);
    auto runOnce = [triggered, autoConnect]() {
        if (*triggered) {
            return;
        }
        *triggered = true;
        autoConnect();
    };

    // Wait for a fresh backend config first, so auto-connect does not start
    // with stale routing profiles from a previous session.
    connect(m_fbLinkController.get(), &FBLinkController::configFetched, this, runOnce, Qt::SingleShotConnection);
    connect(m_fbLinkController.get(), &FBLinkController::configError, this,
            [runOnce](const QString &) { runOnce(); }, Qt::SingleShotConnection);

    // Fallback: do not block auto-connect forever on weak networks.
    QTimer::singleShot(4500, this, runOnce);
}

void CoreController::initFBLinkDnsToggledHandler()
{
    connect(m_settingsController.get(), &SettingsController::fblinkDnsToggled, m_serversModel.get(), &ServersModel::toggleFBLinkDns);
}

void CoreController::initPrepareConfigHandler()
{
    connect(m_connectionController.get(), &ConnectionController::prepareConfig, this, [this]() {
        qDebug() << "[FBLink] prepareConfig: defaultServerIndex =" << m_serversModel->getDefaultServerIndex();

        QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
        const qint64 safeModeUntilEpoch = qSettings.value("Conf/safeModeUntilEpochSec", 0).toLongLong();
        if (safeModeUntilEpoch > QDateTime::currentSecsSinceEpoch()) {
            qWarning() << "[FBLink] prepareConfig: skipped because safe mode is active until" << safeModeUntilEpoch;
            emit m_pageController->showNotificationMessage(tr("Безопасный режим активен. Отключите его на главной странице."));
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        // 1) First check API config validity (this initiates a synchronous HTTP request if expired)
        if (!m_apiConfigsController->isConfigValid()) {
            qDebug() << "[FBLink] prepareConfig: apiConfigsController->isConfigValid() = false";
            emit m_pageController->showNotificationMessage(tr("Ошибка: конфигурация API недействительна"));
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        // 2) Check if containers are installed
        if (!m_installController->isConfigValid()) {
            qDebug() << "[FBLink] prepareConfig: installController->isConfigValid() = false";
            emit m_pageController->showNotificationMessage(tr("Ошибка: нет установленных контейнеров. Войдите в FBLink и получите конфигурацию."));
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            return;
        }

        // 3) Defer connection while backend config/routing sync is in progress.
        // Also force a refresh when managed routing profiles are enabled and
        // the user switched to another FBLink server since the last sync.
        bool requiresServerConfigRefresh = false;
        if (m_fbLinkController && m_fbLinkController->isLoggedIn()) {
            const int enabledManagedProfilesCount = qSettings.value("Conf/vipEnabledProfilesCount", 0).toInt();
            const int defaultServerIndex = m_serversModel->getDefaultServerIndex();
            const int serversCount = m_serversModel->getServersCount();
            if (enabledManagedProfilesCount > 0
                && defaultServerIndex >= 0
                && defaultServerIndex < serversCount) {
                const QJsonObject defaultServer = m_serversModel->getServerConfig(defaultServerIndex);
                const bool isFBLinkServer = defaultServer.value("fblink_server").toBool()
                                            || defaultServer.value(config_key::description).toString().startsWith("FBLink VPN")
                                            || defaultServer.value(config_key::name).toString().startsWith("FBLink VPN");
                if (isFBLinkServer) {
                    const QString currentHost = defaultServer.value(config_key::hostName).toString().trimmed();
                    const QString syncedHost = qSettings.value("Conf/lastSelectedFBLinkHostName", "").toString().trimmed();
                    if (!currentHost.isEmpty() && currentHost.compare(syncedHost, Qt::CaseInsensitive) != 0) {
                        requiresServerConfigRefresh = true;
                        qDebug() << "[FBLink] prepareConfig: selected FBLink host changed from" << syncedHost
                                 << "to" << currentHost << "- forcing fetchConfig()";
                    }
                }
            }
        }

        const bool hasPendingRoutingSync = m_fbLinkController && m_fbLinkController->hasPendingRoutingSync();
        const bool isBackendConfigSyncing = m_fbLinkController && m_fbLinkController->isConfigSyncing();
        const bool isBackendMutationInFlight = m_fbLinkController && m_fbLinkController->isLoading();
        if (m_fbLinkController
            && (isBackendConfigSyncing || hasPendingRoutingSync || isBackendMutationInFlight || requiresServerConfigRefresh)) {
            qDebug() << "[FBLink] prepareConfig: backend config sync is in progress. Waiting for configFetched...";
             
            // Set state to Preparing IMMEDIATELY so the user sees a loading animation while waiting for API
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);
            m_connectionController->setConnectionStateText(tr("Обновление..."));

            if ((hasPendingRoutingSync || requiresServerConfigRefresh) && !isBackendConfigSyncing && !isBackendMutationInFlight) {
                qDebug() << "[FBLink] prepareConfig: forcing fetchConfig() before connect";
                m_fbLinkController->fetchConfig();
            }

            std::shared_ptr<bool> triggered = std::make_shared<bool>(false);

            // When configFetched fires, fresh config is guaranteed — proceed directly
            // without rechecking sync flags (the signal itself is the guarantee).
            auto onConfigFetched = [this, triggered]() {
                if (*triggered) return;
                *triggered = true;
                qDebug() << "[FBLink] prepareConfig: config fetched, calling openConnection()";
                m_connectionController->openConnection();
            };

            auto errorProceed = [this, triggered](const QString &errorText) {
                if (*triggered) return;
                *triggered = true;
                qWarning() << "[FBLink] prepareConfig: config sync failed before connect:" << errorText;
                emit m_pageController->showNotificationMessage(tr("Не удалось обновить конфигурацию перед подключением. Попробуйте ещё раз."));
                emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Error);
            };

            connect(m_fbLinkController.get(), &FBLinkController::configFetched, this, onConfigFetched,
                    static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
            connect(m_fbLinkController.get(), &FBLinkController::configError, this, errorProceed,
                    static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));

            // Fallback timeout: if configFetched hasn't arrived yet (backend is slow),
            // proceed anyway rather than refusing — forcing a double-click was bad UX.
            QTimer::singleShot(7000, this, [this, triggered]() {
                if (*triggered) return;
                *triggered = true;
                qWarning() << "[FBLink] prepareConfig: sync timeout reached, proceeding with cached config";
                m_connectionController->openConnection();
            });
            return;
        }

        // 4) Only AFTER network checks succeed, emit Preparing.
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);

        qDebug() << "[FBLink] prepareConfig: both valid, calling openConnection()";
        m_connectionController->openConnection();
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

void CoreController::openConnectionByIndex(int serverIndex)
{
    if (m_serversModel) {
        m_serversModel->setProcessedServerIndex(serverIndex);
        m_serversModel->setDefaultServerIndex(serverIndex);
    }
    m_connectionController->toggleConnection();
}

void CoreController::importConfigFromData(const QString &data)
{
    if (!m_importController)
        return;

    if (m_importController->extractConfigFromData(data)) {
        m_importController->importConfig();
    }
}
