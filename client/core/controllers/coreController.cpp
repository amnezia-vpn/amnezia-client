#include "coreController.h"

#include <QDirIterator>
#include <QTranslator>

#include "core/utils/selfhosted/sshSession.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/importController.h"
#include "core/controllers/coreSignalHandlers.h"

#if defined(Q_OS_ANDROID)
    #include "core/utils/installedAppsImageProvider.h"
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
    m_usersController = new UsersController(serversRepo, this);
    m_sitesController = new SitesController(appSettingsRepo);
    m_allowedDnsController = new AllowedDnsController(appSettingsRepo);
    m_servicesCatalogController = new ServicesCatalogController(appSettingsRepo);
    m_subscriptionController = new SubscriptionController(serversRepo, appSettingsRepo);
    m_newsController = new NewsController(appSettingsRepo, m_serversController);
    
    SshSession* sshSession = new SshSession(this);
    m_installController = new InstallController(sshSession, serversRepo, m_settings, this);
    m_exportController = new ExportController(serversRepo, appSettingsRepo, m_settings, this);
    m_importCoreController = new ImportController(serversRepo, appSettingsRepo, this);
    m_connectionController = new ConnectionController(serversRepo, appSettingsRepo, m_vpnConnection.get(), m_settings);
    m_settingsController = new SettingsController(serversRepo, appSettingsRepo, this);
}

void CoreController::initControllers()
{
    m_connectionUiController = new ConnectionUiController(m_connectionController, m_serversController, m_containersModel, m_clientManagementModel, m_vpnConnection.get(), this);
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionUiController);

    m_focusController = new FocusController(m_engine, this);
    m_engine->rootContext()->setContextProperty("FocusController", m_focusController);

    m_installUiController = new InstallUiController(m_installController, m_serversController, m_settingsController, m_serversModel, m_containersModel, m_protocolsModel, m_usersController, m_settings, this);
    m_engine->rootContext()->setContextProperty("InstallController", m_installUiController);

    m_importController = new ImportUiController(m_importCoreController, this);
    m_engine->rootContext()->setContextProperty("ImportController", m_importController);

    m_exportUiController = new ExportUiController(m_exportController, this);
    m_engine->rootContext()->setContextProperty("ExportController", m_exportUiController);

    m_languageUiController = new LanguageUiController(m_settingsController, m_languageModel, this);
    m_engine->rootContext()->setContextProperty("LanguageUiController", m_languageUiController);

    m_settingsUiController = new SettingsUiController(m_settingsController, m_serversController, m_containersModel, m_languageUiController, this);
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsUiController);

    m_pageController = new PageController(m_serversModel, m_settingsController, this);
    m_engine->rootContext()->setContextProperty("PageController", m_pageController);

    m_serversUiController = new ServersUiController(m_serversController, m_settingsController, m_serversModel, m_containersModel, m_defaultServerContainersModel, this);
    m_engine->rootContext()->setContextProperty("ServersUiController", m_serversUiController);

    m_sitesUiController = new SitesUiController(m_sitesController, m_vpnConnection.get(), m_sitesModel, this);
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesUiController);

    m_allowedDnsUiController = new AllowedDnsUiController(m_allowedDnsController, m_allowedDnsModel, this);
    m_engine->rootContext()->setContextProperty("AllowedDnsController", m_allowedDnsUiController);

    m_appSplitTunnelingUiController = new AppSplitTunnelingUiController(m_appSplitTunnelingController, m_appSplitTunnelingModel, this);
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingController", m_appSplitTunnelingUiController);

    m_systemController = new SystemController(this);
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController);

    m_servicesCatalogUiController = new ServicesCatalogUiController(m_servicesCatalogController, m_apiServicesModel, this);
    m_engine->rootContext()->setContextProperty("ServicesCatalogUiController", m_servicesCatalogUiController);

    m_subscriptionUiController = new SubscriptionUiController(m_serversController, m_serversModel, m_apiServicesModel, m_servicesCatalogController, m_subscriptionController, m_apiAccountInfoModel, m_apiCountryModel, m_apiDevicesModel, m_settingsController, this);
    m_engine->rootContext()->setContextProperty("SubscriptionUiController", m_subscriptionUiController);
    m_engine->rootContext()->setContextProperty("SubscriptionUiController", m_subscriptionUiController);

    m_apiNewsUiController = new ApiNewsUiController(m_newsModel, m_newsController, this);
    m_engine->rootContext()->setContextProperty("ApiNewsController", m_apiNewsUiController);
}

void CoreController::initAndroidController()
{
#ifdef Q_OS_ANDROID
    if (!AndroidController::initLogging()) {
        qFatal("Android logging initialization failed");
    }
    AndroidController::instance()->setSaveLogs(m_appSettingsRepository->isSaveLogs());
    AndroidController::instance()->setScreenshotsEnabled(m_appSettingsRepository->isScreenshotsEnabled());

    if (!AndroidController::instance()->initialize()) {
        qFatal("Android controller initialization failed");
    }

    initAndroidSettingsHandler();
    initAndroidConnectionHandler();

    m_engine->addImageProvider(QLatin1String("installedAppImage"), new InstalledAppsImageProvider);
#endif
}

void CoreController::initAppleController()
{
#ifdef Q_OS_IOS
    IosController::Instance()->initialize();
    QTimer::singleShot(0, this, [this]() { AmneziaVPN::toggleScreenshots(m_appSettingsRepository->isScreenshotsEnabled()); });
#endif
}

void CoreController::initSignalHandlers()
{
    m_signalHandlers = new CoreSignalHandlers(this, this);
    m_signalHandlers->initAllHandlers();
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

void CoreController::setQmlRoot()
{
    m_systemController->setQmlRoot(m_engine->rootObjects().value(0));
}

PageController* CoreController::pageController() const
{
    return m_pageController;
}

void CoreController::openConnectionByIndex(int serverIndex)
{
    if (m_serversModel) {
        m_serversModel->setProcessedServerIndex(serverIndex);
    }
    if (m_serversController) {
        m_serversController->setDefaultServerIndex(serverIndex);
    }
    m_connectionUiController->toggleConnection();
}

void CoreController::importConfigFromData(const QString &data)
{
    if (!m_importController)
        return;

    if (m_importController->extractConfigFromData(data)) {
        m_importController->importConfig();
    }
}
