#ifndef TESTCORECONTROLLER_H
#define TESTCORECONTROLLER_H

#include "core/controllers/coreController.h"

class TestCoreController final : public CoreController
{
public:
    explicit TestCoreController(const QSharedPointer<VpnConnection> &vpnConnection,
                                SecureQSettings *settings,
                                QQmlApplicationEngine *engine,
                                QObject *parent = nullptr)
        : CoreController(vpnConnection, settings, engine, parent, /*skipPlatformControllerInit=*/true)
        , m_serversRepository(serversRepositoryProtected())
        , m_appSettingsRepository(appSettingsRepositoryProtected())
        , m_serversModel(serversModelProtected())
        , m_containersModel(containersModelProtected())
        , m_apiServicesModel(apiServicesModelProtected())
        , m_newsModel(newsModelProtected())
        , m_allowedDnsModel(allowedDnsModelProtected())
        , m_appSplitTunnelingModel(appSplitTunnelingModelProtected())
        , m_ipSplitTunnelingModel(ipSplitTunnelingModelProtected())
        , m_languageModel(languageModelProtected())
        , m_connectionUiController(connectionUiControllerProtected())
        , m_installUiController(installUiControllerProtected())
        , m_importCoreController(importCoreControllerProtected())
        , m_exportController(exportControllerProtected())
        , m_installController(installControllerProtected())
        , m_serversController(serversControllerProtected())
        , m_settingsUiController(settingsUiControllerProtected())
        , m_settingsController(settingsControllerProtected())
        , m_allowedDnsUiController(allowedDnsUiControllerProtected())
        , m_allowedDnsController(allowedDnsControllerProtected())
        , m_languageUiController(languageUiControllerProtected())
        , m_ipSplitTunnelingController(ipSplitTunnelingControllerProtected())
        , m_ipSplitTunnelingUiController(ipSplitTunnelingUiControllerProtected())
        , m_appSplitTunnelingController(appSplitTunnelingControllerProtected())
        , m_appSplitTunnelingUiController(appSplitTunnelingUiControllerProtected())
        , m_serversUiController(serversUiControllerProtected())
        , m_servicesCatalogUiController(servicesCatalogUiControllerProtected())
        , m_apiNewsUiController(apiNewsUiControllerProtected())
    {
    }

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
    ServersModel *m_serversModel;
    ContainersModel *m_containersModel;
    ApiServicesModel *m_apiServicesModel;
    NewsModel *m_newsModel;
    AllowedDnsModel *m_allowedDnsModel;
    AppSplitTunnelingModel *m_appSplitTunnelingModel;
    IpSplitTunnelingModel *m_ipSplitTunnelingModel;
    LanguageModel *m_languageModel;

    ConnectionUiController *m_connectionUiController;
    InstallUiController *m_installUiController;
    ImportController *m_importCoreController;
    ExportController *m_exportController;
    InstallController *m_installController;
    ServersController *m_serversController;
    SettingsUiController *m_settingsUiController;
    SettingsController *m_settingsController;
    AllowedDnsUiController *m_allowedDnsUiController;
    AllowedDnsController *m_allowedDnsController;
    LanguageUiController *m_languageUiController;
    IpSplitTunnelingController *m_ipSplitTunnelingController;
    IpSplitTunnelingUiController *m_ipSplitTunnelingUiController;
    AppSplitTunnelingController *m_appSplitTunnelingController;
    AppSplitTunnelingUiController *m_appSplitTunnelingUiController;
    ServersUiController *m_serversUiController;
    ServicesCatalogUiController *m_servicesCatalogUiController;
    ApiNewsUiController *m_apiNewsUiController;
};

#endif // TESTCORECONTROLLER_H
