#ifndef CORECONTROLLER_H
#define CORECONTROLLER_H

#include <QObject>
#include <QQmlContext>
#include <QThread>

#ifndef Q_OS_ANDROID
    #include "ui/systemtray_notificationhandler.h"
#endif

#include "ui/controllers/api/apiConfigsController.h"
#include "ui/controllers/api/apiSettingsController.h"
#include "ui/controllers/api/apiPremV1MigrationController.h"
#include "ui/controllers/appSplitTunnelingController.h"
#include "ui/controllers/allowedDnsController.h"
#include "ui/controllers/connectionController.h"
#include "ui/controllers/exportController.h"
#include "ui/controllers/focusController.h"
#include "ui/controllers/importController.h"
#include "ui/controllers/installController.h"
#include "ui/controllers/pageController.h"
#include "ui/controllers/settingsController.h"
#include "ui/controllers/sitesController.h"
#include "ui/controllers/systemController.h"

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

#ifndef Q_OS_ANDROID
    #include "ui/notificationhandler.h"
#endif

class CoreController : public QObject
{
    Q_OBJECT

public:
    explicit CoreController(const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                            QQmlApplicationEngine *engine, QObject *parent = nullptr);

    QSharedPointer<PageController> pageController() const;
    void setQmlRoot();

signals:
    void translationsUpdated();
    void websiteUrlChanged(const QString &newUrl);

private:
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
    void initAutoConnectHandler();
    void initAmneziaDnsToggledHandler();
    void initPrepareConfigHandler();
    void initImportPremiumV2VpnKeyHandler();
    void initShowMigrationDrawerHandler();
    void initStrictKillSwitchHandler();

    QQmlApplicationEngine *m_engine {}; // TODO use parent child system here?
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<VpnConnection> m_vpnConnection;
    QSharedPointer<QTranslator> m_translator;

#ifndef Q_OS_ANDROID
    QScopedPointer<NotificationHandler> m_notificationHandler;
#endif

    QMetaObject::Connection m_reloadConfigErrorOccurredConnection;

    QScopedPointer<ConnectionController> m_connectionController;
    QScopedPointer<FocusController> m_focusController;
    QSharedPointer<PageController> m_pageController; // TODO
    QScopedPointer<InstallController> m_installController;
    QScopedPointer<ImportController> m_importController;
    QScopedPointer<ExportController> m_exportController;
    QScopedPointer<SettingsController> m_settingsController;
    QScopedPointer<SitesController> m_sitesController;
    QScopedPointer<SystemController> m_systemController;
    QScopedPointer<AppSplitTunnelingController> m_appSplitTunnelingController;
    QScopedPointer<AllowedDnsController> m_allowedDnsController;

    QScopedPointer<ApiSettingsController> m_apiSettingsController;
    QScopedPointer<ApiConfigsController> m_apiConfigsController;
    QScopedPointer<ApiPremV1MigrationController> m_apiPremV1MigrationController;

    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ContainersModel> m_defaultServerContainersModel;
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<LanguageModel> m_languageModel;
    QSharedPointer<ProtocolsModel> m_protocolsModel;
    QSharedPointer<SitesModel> m_sitesModel;
    QSharedPointer<AllowedDnsModel> m_allowedDnsModel;
    QSharedPointer<AppSplitTunnelingModel> m_appSplitTunnelingModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;

    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    QSharedPointer<ApiCountryModel> m_apiCountryModel;
    QSharedPointer<ApiAccountInfoModel> m_apiAccountInfoModel;
    QSharedPointer<ApiDevicesModel> m_apiDevicesModel;

    QScopedPointer<OpenVpnConfigModel> m_openVpnConfigModel;
    QScopedPointer<ShadowSocksConfigModel> m_shadowSocksConfigModel;
    QScopedPointer<CloakConfigModel> m_cloakConfigModel;
    QScopedPointer<XrayConfigModel> m_xrayConfigModel;
    QScopedPointer<WireGuardConfigModel> m_wireGuardConfigModel;
    QScopedPointer<AwgConfigModel> m_awgConfigModel;
#ifdef Q_OS_WINDOWS
    QScopedPointer<Ikev2ConfigModel> m_ikev2ConfigModel;
#endif
    QScopedPointer<SftpConfigModel> m_sftpConfigModel;
    QScopedPointer<Socks5ProxyConfigModel> m_socks5ConfigModel;
};

#endif // CORECONTROLLER_H
