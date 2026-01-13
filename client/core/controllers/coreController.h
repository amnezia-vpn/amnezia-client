#ifndef CORECONTROLLER_H
#define CORECONTROLLER_H

#include <QObject>
#include <QQmlContext>
#include <QThread>

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    #include "ui/utils/systemTrayNotificationHandler.h"
#endif

#include "ui/controllers/api/subscriptionUiController.h"
#include "ui/controllers/api/apiPremV1MigrationController.h"
#include "ui/controllers/api/apiNewsUiController.h"
#include "ui/controllers/appSplitTunnelingUiController.h"
#include "ui/controllers/allowedDnsUiController.h"
#include "ui/controllers/connectionController.h"
#include "ui/controllers/selfhosted/exportUiController.h"
#include "core/controllers/exportController.h"
#include "ui/controllers/qml/focusController.h"
#include "ui/controllers/importController.h"
#include "ui/controllers/selfhosted/installUiController.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/controllers/selfhosted/protocolsUiController.h"
#include "ui/controllers/settingsController.h"
#include "ui/controllers/serversUiController.h"
#include "ui/controllers/sitesUiController.h"
#include "ui/controllers/systemController.h"
#include "ui/controllers/appSplitTunnelingUiController.h"
#include "ui/controllers/allowedDnsUiController.h"
#include "ui/controllers/languageUiController.h"
#include "ui/controllers/api/servicesCatalogUiController.h"

#include "core/controllers/serversController.h"
#include "core/controllers/clientManagementController.h"
#include "core/controllers/appSplitTunnelingController.h"
#include "core/controllers/sitesController.h"
#include "core/controllers/allowedDnsController.h"
#include "core/controllers/api/servicesCatalogController.h"
#include "core/controllers/api/subscriptionController.h"
#include "core/controllers/api/newsController.h"
#include "core/controllers/installController.h"

#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"

#include "ui/models/allowed_dns_model.h"
#include "ui/models/containers_model.h"
#include "ui/models/languageModel.h"
#include "ui/models/protocols/cloakConfigModel.h"
#ifdef Q_OS_WINDOWS
    #include "ui/models/protocols/ikev2ConfigModel.h"
#endif
#include "ui/models/api/apiAccountInfoModel.h"
#include "ui/models/api/apiCountryModel.h"
#include "ui/models/api/apiDevicesModel.h"
#include "ui/models/api/apiServicesModel.h"
#include "ui/models/appSplitTunnelingModel.h"
#include "ui/models/clientManagementModel.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/openvpnConfigModel.h"
#include "ui/models/protocols/shadowsocksConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "ui/models/protocols_model.h"
#include "ui/models/servers_model.h"
#include "ui/models/services/sftpConfigModel.h"
#include "ui/models/services/socks5ProxyConfigModel.h"
#include "ui/models/sites_model.h"
#include "ui/models/newsModel.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    #include "ui/utils/notificationHandler.h"
#endif

class CoreController : public QObject
{
    Q_OBJECT

public:
    explicit CoreController(const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                            QQmlApplicationEngine *engine, QObject *parent = nullptr);

    PageController* pageController() const;
    void setQmlRoot();

    void openConnectionByIndex(int serverIndex);
    void importConfigFromData(const QString &data);

signals:
    void translationsUpdated();
    void websiteUrlChanged(const QString &newUrl);

private:
    void initRepositories();
    void initCoreControllers();
    void initModels();
    void initControllers();
    void initAndroidController();
    void initAppleController();
    void initSignalHandlers();

    void initNotificationHandler();

    void updateTranslator(const QLocale &locale);

    void initErrorMessagesHandler();

    void initApiCountryModelUpdateHandler();
    void initContainerModelUpdateHandler();
    void initAdminConfigRevokedHandler();
    void initPassphraseRequestHandler();
    void initTranslationsUpdatedHandler();
    void initLanguageHandler();
    void initAutoConnectHandler();
    void initAmneziaDnsToggledHandler();
    void initServersModelUpdateHandler();
    void initClientManagementModelUpdateHandler();
    void initSitesModelUpdateHandler();
    void initAllowedDnsModelUpdateHandler();
    void initAppSplitTunnelingModelUpdateHandler();
    void initPrepareConfigHandler();
    void initImportPremiumV2VpnKeyHandler();
    void initShowMigrationDrawerHandler();
    void initStrictKillSwitchHandler();

    QQmlApplicationEngine *m_engine {}; // TODO use parent child system here?
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<VpnConnection> m_vpnConnection;
    QTranslator* m_translator;

    QServersRepository* m_serversRepository;
    QAppSettingsRepository* m_appSettingsRepository;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    NotificationHandler* m_notificationHandler;
#endif

    QMetaObject::Connection m_reloadConfigErrorOccurredConnection;

    ConnectionController* m_connectionController;
    FocusController* m_focusController;
    PageController* m_pageController;
    InstallUiController* m_installUiController;
    ImportController* m_importController;
    ExportUiController* m_exportUiController;
    SettingsController* m_settingsController;
    ServersUiController* m_serversUiController;
    SitesUiController* m_sitesUiController;
    SystemController* m_systemController;
    AppSplitTunnelingUiController* m_appSplitTunnelingUiController;
    AllowedDnsUiController* m_allowedDnsUiController;
    LanguageUiController* m_languageUiController;

    SubscriptionUiController* m_subscriptionUiController;
    ApiPremV1MigrationController* m_apiPremV1MigrationController;
    ApiNewsUiController* m_apiNewsUiController;
    
    ProtocolsUiController* m_protocolsUiController;
    ServicesCatalogUiController* m_servicesCatalogUiController;

    ServersController* m_serversController;
    ClientManagementController* m_clientManagementController;
    AppSplitTunnelingController* m_appSplitTunnelingController;
    SitesController* m_sitesController;
    AllowedDnsController* m_allowedDnsController;
    ServicesCatalogController* m_servicesCatalogController;
    SubscriptionController* m_subscriptionController;
    NewsController* m_newsController;
    InstallController* m_installController;
    ExportController* m_exportController;

    ContainersModel* m_containersModel;
    ContainersModel* m_defaultServerContainersModel;
    ServersModel* m_serversModel;
    LanguageModel* m_languageModel;
    ProtocolsModel* m_protocolsModel;
    SitesModel* m_sitesModel;
    NewsModel* m_newsModel;
    AllowedDnsModel* m_allowedDnsModel;
    AppSplitTunnelingModel* m_appSplitTunnelingModel;
    ClientManagementModel* m_clientManagementModel;

    ApiServicesModel* m_apiServicesModel;
    ApiCountryModel* m_apiCountryModel;
    ApiAccountInfoModel* m_apiAccountInfoModel;
    ApiDevicesModel* m_apiDevicesModel;

    OpenVpnConfigModel* m_openVpnConfigModel;
    ShadowSocksConfigModel* m_shadowSocksConfigModel;
    CloakConfigModel* m_cloakConfigModel;
    XrayConfigModel* m_xrayConfigModel;
    WireGuardConfigModel* m_wireGuardConfigModel;
    AwgConfigModel* m_awgConfigModel;
#ifdef Q_OS_WINDOWS
    Ikev2ConfigModel* m_ikev2ConfigModel;
#endif
    SftpConfigModel* m_sftpConfigModel;
    Socks5ProxyConfigModel* m_socks5ConfigModel;
};

#endif // CORECONTROLLER_H
