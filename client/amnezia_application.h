#ifndef AMNEZIA_APPLICATION_H
#define AMNEZIA_APPLICATION_H

#include <QCommandLineParser>
#include <QNetworkAccessManager>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "settings.h"
#include "vpnconnection.h"

#include "ui/controllers/connectionController.h"
#include "ui/controllers/exportController.h"
#include "ui/controllers/importController.h"
#include "ui/controllers/installController.h"
#include "ui/controllers/focusController.h"
#include "ui/controllers/pageController.h"
#include "ui/controllers/settingsController.h"
#include "ui/controllers/sitesController.h"
#include "ui/controllers/systemController.h"
#include "ui/controllers/appSplitTunnelingController.h"
#include "ui/models/containers_model.h"
#include "ui/models/languageModel.h"
#include "ui/models/protocols/cloakConfigModel.h"
#ifndef Q_OS_ANDROID
    #include "ui/notificationhandler.h"
#endif
#ifdef Q_OS_WINDOWS
    #include "ui/models/protocols/ikev2ConfigModel.h"
#endif
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
#include "ui/models/clientManagementModel.h"
#include "ui/models/appSplitTunnelingModel.h"
#include "ui/models/apiServicesModel.h"
#include "ui/models/apiCountryModel.h"

#define amnApp (static_cast<AmneziaApplication *>(QCoreApplication::instance()))

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #define AMNEZIA_BASE_CLASS QGuiApplication
#else
    #define AMNEZIA_BASE_CLASS QApplication
#endif

class AmneziaApplication : public AMNEZIA_BASE_CLASS
{
    Q_OBJECT
public:
    AmneziaApplication(int &argc, char *argv[]);
    virtual ~AmneziaApplication();

    void init();
    void registerTypes();
    void loadFonts();
    void loadTranslator();
    void updateTranslator(const QLocale &locale);
    bool parseCommands();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void startLocalServer();
#endif

    QQmlApplicationEngine *qmlEngine() const;
    QNetworkAccessManager *manager() { return m_nam; }

signals:
    void translationsUpdated();

private:
    void initModels();
    void initControllers();

    QQmlApplicationEngine *m_engine {};
    std::shared_ptr<Settings> m_settings;

    QSharedPointer<ContainerProps> m_containerProps;
    QSharedPointer<ProtocolProps> m_protocolProps;

    QSharedPointer<QTranslator> m_translator;
    QCommandLineParser m_parser;

    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ContainersModel> m_defaultServerContainersModel;
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<LanguageModel> m_languageModel;
    QSharedPointer<ProtocolsModel> m_protocolsModel;
    QSharedPointer<SitesModel> m_sitesModel;
    QSharedPointer<AppSplitTunnelingModel> m_appSplitTunnelingModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;
    QSharedPointer<ApiServicesModel> m_apiServicesModel;
    QSharedPointer<ApiCountryModel> m_apiCountryModel;

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

    QSharedPointer<VpnConnection> m_vpnConnection;
    QThread m_vpnConnectionThread;
#ifndef Q_OS_ANDROID
    QScopedPointer<NotificationHandler> m_notificationHandler;
#endif

    QScopedPointer<ConnectionController> m_connectionController;
    QScopedPointer<FocusController> m_focusController;
    QScopedPointer<PageController> m_pageController;
    QScopedPointer<InstallController> m_installController;
    QScopedPointer<ImportController> m_importController;
    QScopedPointer<ExportController> m_exportController;
    QScopedPointer<SettingsController> m_settingsController;
    QScopedPointer<SitesController> m_sitesController;
    QScopedPointer<SystemController> m_systemController;
    QScopedPointer<AppSplitTunnelingController> m_appSplitTunnelingController;

    QNetworkAccessManager *m_nam;

    QMetaObject::Connection m_reloadConfigErrorOccurredConnection;
};

#endif // AMNEZIA_APPLICATION_H
