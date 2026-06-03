#include "coreController.h"

#include <QDirIterator>
#include <QTranslator>
#include <QTimer>

#include "core/utils/selfhosted/sshSession.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/importController.h"
#include "core/controllers/coreSignalHandlers.h"
#include "logger.h"
#include "secureQSettings.h"

#if defined(Q_OS_ANDROID)
    #include "core/utils/installedAppsImageProvider.h"
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/ios_controller.h"
    #include <AmneziaVPN-Swift.h>
#endif

CoreController::CoreController(const QSharedPointer<VpnConnection> &vpnConnection, SecureQSettings* settings,
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
    initLogging();

    m_translator = new QTranslator(this);
    if (m_appSettingsRepository) {
        updateTranslator(m_appSettingsRepository->getAppLanguage());
    }
}

void CoreController::setQmlContextProperty(const QString &name, QObject *value)
{
    if (m_engine) {
        m_engine->rootContext()->setContextProperty(name, value);
    }
}

void CoreController::initModels()
{
    m_containersModel = new ContainersModel(this);
    setQmlContextProperty("ContainersModel", m_containersModel);

    m_defaultServerContainersModel = new ContainersModel(this);
    setQmlContextProperty("DefaultServerContainersModel", m_defaultServerContainersModel);

    m_serversModel = new ServersModel(this);
    setQmlContextProperty("ServersModel", m_serversModel);

    m_languageModel = new LanguageModel(this);
    setQmlContextProperty("LanguageModel", m_languageModel);

    m_ipSplitTunnelingModel = new IpSplitTunnelingModel(this);
    setQmlContextProperty("IpSplitTunnelingModel", m_ipSplitTunnelingModel);

    m_allowedDnsModel = new AllowedDnsModel(this);
    setQmlContextProperty("AllowedDnsModel", m_allowedDnsModel);

    m_appSplitTunnelingModel = new AppSplitTunnelingModel(this);
    setQmlContextProperty("AppSplitTunnelingModel", m_appSplitTunnelingModel);

    m_protocolsModel = new ProtocolsModel(this);
    setQmlContextProperty("ProtocolsModel", m_protocolsModel);

    m_openVpnConfigModel = new OpenVpnConfigModel(this);
    setQmlContextProperty("OpenVpnConfigModel", m_openVpnConfigModel);

    m_wireGuardConfigModel = new WireGuardConfigModel(this);
    setQmlContextProperty("WireGuardConfigModel", m_wireGuardConfigModel);

    m_awgConfigModel = new AwgConfigModel(this);
    setQmlContextProperty("AwgConfigModel", m_awgConfigModel);

    m_xrayConfigModel = new XrayConfigModel(this);
    setQmlContextProperty("XrayConfigModel", m_xrayConfigModel);

    m_xrayConfigSnapshotsModel = new XrayConfigSnapshotsModel(m_appSettingsRepository, m_xrayConfigModel, this);
    setQmlContextProperty("XrayConfigSnapshotsModel", m_xrayConfigSnapshotsModel);

    m_torConfigModel = new TorConfigModel(this);
    setQmlContextProperty("TorConfigModel", m_torConfigModel);

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel = new Ikev2ConfigModel(this);
    setQmlContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel);
#endif

    m_sftpConfigModel = new SftpConfigModel(this);
    setQmlContextProperty("SftpConfigModel", m_sftpConfigModel);

    m_socks5ConfigModel = new Socks5ProxyConfigModel(this);
    setQmlContextProperty("Socks5ProxyConfigModel", m_socks5ConfigModel);

    m_mtProxyConfigModel = new MtProxyConfigModel(this);
    setQmlContextProperty("MtProxyConfigModel", m_mtProxyConfigModel);

    m_telemtConfigModel = new TelemtConfigModel(this);
    setQmlContextProperty("TelemtConfigModel", m_telemtConfigModel);

    m_clientManagementModel = new ClientManagementModel(this);
    setQmlContextProperty("ClientManagementModel", m_clientManagementModel);

    m_apiServicesModel = new ApiServicesModel(this);
    setQmlContextProperty("ApiServicesModel", m_apiServicesModel);

    m_apiCountryModel = new ApiCountryModel(this);
    setQmlContextProperty("ApiCountryModel", m_apiCountryModel);

    m_apiSubscriptionPlansModel = new ApiSubscriptionPlansModel(this);
    setQmlContextProperty("ApiSubscriptionPlansModel", m_apiSubscriptionPlansModel);

    m_apiBenefitsModel = new ApiBenefitsModel(this);
    setQmlContextProperty("ApiBenefitsModel", m_apiBenefitsModel);

    m_apiAccountInfoModel = new ApiAccountInfoModel(this);
    setQmlContextProperty("ApiAccountInfoModel", m_apiAccountInfoModel);

    m_apiDevicesModel = new ApiDevicesModel(this);
    setQmlContextProperty("ApiDevicesModel", m_apiDevicesModel);

    m_newsModel = new NewsModel(m_appSettingsRepository, this);
    setQmlContextProperty("NewsModel", m_newsModel);
}

void CoreController::initRepositories()
{
    m_serversRepository = new SecureServersRepository(m_settings, this);
    m_appSettingsRepository = new SecureAppSettingsRepository(m_settings, this);

    if (m_vpnConnection) {
        m_vpnConnection->setRepositories(m_serversRepository, m_appSettingsRepository);
    }
}

void CoreController::initCoreControllers()
{
    m_serversController = new ServersController(m_serversRepository, m_appSettingsRepository, this);
    m_appSplitTunnelingController = new AppSplitTunnelingController(m_appSettingsRepository);
    m_usersController = new UsersController(m_serversRepository, this);
    m_ipSplitTunnelingController = new IpSplitTunnelingController(m_appSettingsRepository, this);
    m_allowedDnsController = new AllowedDnsController(m_appSettingsRepository);
    m_servicesCatalogController = new ServicesCatalogController(m_appSettingsRepository);
    m_subscriptionController = new SubscriptionController(m_serversRepository, m_appSettingsRepository);
    m_newsController = new NewsController(m_appSettingsRepository, m_serversRepository);
    m_updateController = new UpdateController(m_appSettingsRepository, this);
    
    m_installController = new InstallController(m_serversRepository, m_appSettingsRepository, this);
    m_exportController = new ExportController(m_serversRepository, m_appSettingsRepository, this);
    m_importCoreController = new ImportController(m_serversRepository, m_appSettingsRepository, this);
    m_connectionController = new ConnectionController(m_serversRepository, m_appSettingsRepository, m_vpnConnection.get(), this);
    m_settingsController = new SettingsController(m_serversRepository, m_appSettingsRepository, this);
}

void CoreController::initControllers()
{
    m_connectionUiController = new ConnectionUiController(m_connectionController, m_serversController, this);
    setQmlContextProperty("ConnectionController", m_connectionUiController);

    if (m_engine) {
        m_focusController = new FocusController(m_engine, this);
        setQmlContextProperty("FocusController", m_focusController);
    }

    m_installUiController = new InstallUiController(m_installController, m_serversController, m_settingsController, m_protocolsModel, m_usersController,
                                                     m_awgConfigModel, m_wireGuardConfigModel, m_openVpnConfigModel, m_xrayConfigModel, m_torConfigModel,
#ifdef Q_OS_WINDOWS
                                                     m_ikev2ConfigModel,
#endif
                                                     m_sftpConfigModel, m_socks5ConfigModel, m_mtProxyConfigModel, m_telemtConfigModel,
                                                     m_connectionController, this);
    setQmlContextProperty("InstallController", m_installUiController);

    m_importController = new ImportUiController(m_importCoreController, this);
    setQmlContextProperty("ImportController", m_importController);

    m_exportUiController = new ExportUiController(m_exportController, this);
    setQmlContextProperty("ExportController", m_exportUiController);

    m_languageUiController = new LanguageUiController(m_settingsController, m_languageModel, this);
    setQmlContextProperty("LanguageUiController", m_languageUiController);

    m_settingsUiController = new SettingsUiController(m_settingsController, m_serversController, this);
    setQmlContextProperty("SettingsController", m_settingsUiController);

    m_pageController = new PageController(m_serversController, m_settingsController, this);
    setQmlContextProperty("PageController", m_pageController);

    m_serversUiController = new ServersUiController(m_serversController, m_settingsController, m_serversModel, m_containersModel, m_defaultServerContainersModel, this);
    setQmlContextProperty("ServersUiController", m_serversUiController);

    m_ipSplitTunnelingUiController = new IpSplitTunnelingUiController(m_ipSplitTunnelingController, m_ipSplitTunnelingModel, this);
    setQmlContextProperty("IpSplitTunnelingController", m_ipSplitTunnelingUiController);

    m_allowedDnsUiController = new AllowedDnsUiController(m_allowedDnsController, m_allowedDnsModel, this);
    setQmlContextProperty("AllowedDnsController", m_allowedDnsUiController);

    m_appSplitTunnelingUiController = new AppSplitTunnelingUiController(m_appSplitTunnelingController, m_appSplitTunnelingModel, this);
    setQmlContextProperty("AppSplitTunnelingController", m_appSplitTunnelingUiController);

    m_systemController = new SystemController(this);
    setQmlContextProperty("SystemController", m_systemController);

    m_networkReachabilityController = new NetworkReachabilityController(this);
    setQmlContextProperty("NetworkReachabilityController", m_networkReachabilityController);
    setQmlContextProperty("NetworkReachability", m_networkReachabilityController);

    m_servicesCatalogUiController = new ServicesCatalogUiController(m_servicesCatalogController, m_apiServicesModel, this);
    setQmlContextProperty("ServicesCatalogUiController", m_servicesCatalogUiController);

    m_subscriptionUiController = new SubscriptionUiController(m_serversController, m_apiServicesModel, m_servicesCatalogController, m_subscriptionController,
                                                              m_apiSubscriptionPlansModel, m_apiBenefitsModel, m_apiAccountInfoModel,
                                                              m_apiCountryModel, m_apiDevicesModel, m_settingsController,
                                                              m_connectionController, this);
    setQmlContextProperty("SubscriptionUiController", m_subscriptionUiController);

    m_apiNewsUiController = new ApiNewsUiController(m_newsModel, m_newsController, this);
    setQmlContextProperty("ApiNewsController", m_apiNewsUiController);

    m_updateUiController = new UpdateUiController(m_updateController, this);
    setQmlContextProperty("UpdateController", m_updateUiController);
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

    if (m_engine) {
        m_engine->addImageProvider(QLatin1String("installedAppImage"), new InstalledAppsImageProvider);
    }
#endif
}

void CoreController::initAppleController()
{
#ifdef Q_OS_IOS
    IosController::Instance()->initialize();
    QTimer::singleShot(0, this, [this]() { AmneziaVPN::toggleScreenshots(m_appSettingsRepository->isScreenshotsEnabled()); });
#endif
}

void CoreController::initLogging()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    bool enabled = m_appSettingsRepository->isSaveLogs();
    if (enabled) {
        if (!Logger::init(false)) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }
    Logger::setServiceLogsEnabled(enabled);
#endif
}

void CoreController::initSignalHandlers()
{
    m_signalHandlers = new CoreSignalHandlers(this, this);
    m_signalHandlers->initAllHandlers();

    // Trigger initial update after handlers are connected
    m_serversUiController->updateModel();
    if (m_serversUiController->hasServersFromGatewayApi()) {
        m_apiNewsUiController->fetchNews(false);
    }
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

    if (m_engine) {
        m_engine->retranslate();
    }

    emit translationsUpdated();
    if (m_languageUiController) {
        emit websiteUrlChanged(m_languageUiController->getCurrentSiteUrl());
    }
}

void CoreController::setQmlRoot()
{
    if (m_engine && m_systemController) {
        m_systemController->setQmlRoot(m_engine->rootObjects().value(0));
    }
}

PageController* CoreController::pageController() const
{
    return m_pageController;
}

void CoreController::openConnectionByIndex(int serverIndex)
{
    const QString serverId =
        m_serversUiController ? m_serversUiController->getServerId(serverIndex) : QString();
    if (serverId.isEmpty()) {
        return;
    }
    if (m_serversController) {
        m_serversController->setDefaultServer(serverId);
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
