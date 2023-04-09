#ifndef UILOGIC_H
#define UILOGIC_H

#include <QFileDialog>
#include <QKeyEvent>
#include <QRegularExpressionValidator>
#include <QThread>
#include <QQmlEngine>

#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "property_helper.h"
#include "pages.h"
#include "protocols/vpnprotocol.h"
#include "containers/containers_defs.h"

#include "models/containers_model.h"
#include "models/protocols_model.h"

#include "notificationhandler.h"

class Settings;
class VpnConfigurator;
class ServerController;

class PageLogicBase;

class AppSettingsLogic;
class GeneralSettingsLogic;
class NetworkSettingsLogic;
class NewServerProtocolsLogic;
class QrDecoderLogic;
class ServerConfiguringProgressLogic;
class ServerListLogic;
class ServerSettingsLogic;
class ServerContainersLogic;
class ShareConnectionLogic;
class SitesLogic;
class StartPageLogic;
class ViewConfigLogic;
class VpnLogic;
class WizardLogic;
class AdvancedServerSettingsLogic;

class PageProtocolLogicBase;
class OpenVpnLogic;
class ShadowSocksLogic;
class CloakLogic;

class OtherProtocolsLogic;

class VpnConnection;

class CreateServerTest;

class UiLogic : public QObject
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pageEnabled)
    AUTO_PROPERTY(int, pagesStackDepth)
    AUTO_PROPERTY(int, currentPageValue)
    AUTO_PROPERTY(QString, popupWarningText)

    READONLY_PROPERTY(QObject *, containersModel)
    READONLY_PROPERTY(QObject *, protocolsModel)

public:
    explicit UiLogic(std::shared_ptr<Settings> settings, std::shared_ptr<VpnConfigurator> configurator,
        std::shared_ptr<ServerController> serverController, QObject *parent = nullptr);
    ~UiLogic();
    void showOnStartup();

    friend class PageLogicBase;

    friend class AppSettingsLogic;
    friend class GeneralSettingsLogic;
    friend class NetworkSettingsLogic;
    friend class ServerConfiguringProgressLogic;
    friend class NewServerProtocolsLogic;
    friend class ServerListLogic;
    friend class ServerSettingsLogic;
    friend class ServerContainersLogic;
    friend class ShareConnectionLogic;
    friend class SitesLogic;
    friend class StartPageLogic;
    friend class ViewConfigLogic;
    friend class VpnLogic;
    friend class WizardLogic;
    friend class AdvancedServerSettingsLogic;

    friend class PageProtocolLogicBase;
    friend class OpenVpnLogic;
    friend class ShadowSocksLogic;
    friend class CloakLogic;

    friend class OtherProtocolsLogic;

    friend class CreateServerTest;

    Q_INVOKABLE virtual void onUpdatePage() {} // UiLogic is set as logic class for some qml pages
    Q_INVOKABLE void onUpdateAllPages();

    Q_INVOKABLE void initializeUiLogic();
    Q_INVOKABLE void onCloseWindow();

    Q_INVOKABLE QString containerName(int container);
    Q_INVOKABLE QString containerDesc(int container);

    Q_INVOKABLE void onGotoCurrentProtocolsPage();

    Q_INVOKABLE void keyPressEvent(Qt::Key key);

    Q_INVOKABLE void saveTextFile(const QString& desc, const QString &suggestedName, QString ext, const QString& data);
    Q_INVOKABLE void saveBinaryFile(const QString& desc, QString ext, const QString& data);
    Q_INVOKABLE void copyToClipboard(const QString& text);

    Q_INVOKABLE amnezia::ErrorCode addAlreadyInstalledContainersGui(bool &isServerCreated);

    void shareTempFile(const QString &suggestedName, QString ext, const QString& data);
    static QString getOpenFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = QFileDialog::Options());
signals:
    void goToPage(PageEnumNS::Page page, bool reset = true, bool slide = true);
    void goToProtocolPage(Proto protocol, bool reset = true, bool slide = true);
    void goToShareProtocolPage(Proto protocol, bool reset = true, bool slide = true);

    void closePage();
    void setStartPage(PageEnumNS::Page page, bool slide = true);
    void showPublicKeyWarning();
    void showConnectErrorDialog();
    void show();
    void hide();
    void raise();
    void toggleLogPanel();
    void showWarningMessage(QString message);

private slots:
    // containers - INOUT arg
    void installServer(QPair<amnezia::DockerContainer, QJsonObject> &container);

private:
    PageEnumNS::Page currentPage();
    bool isContainerAlreadyAddedToGui(DockerContainer container);

public:
    Q_INVOKABLE PageProtocolLogicBase *protocolLogic(Proto p);

    QObject *qmlRoot() const;
    void setQmlRoot(QObject *newQmlRoot);

    NotificationHandler *notificationHandler() const;

    void setQmlContextProperty(PageLogicBase *logic);
    void registerPagesLogic();

    template<typename T>
    void registerPageLogic()
    {
        T* logic = new T(this);
        m_logicMap[std::type_index(typeid(T))] = logic;
        setQmlContextProperty(logic);
    }

    template<typename T>
    T* pageLogic()
    {
        return static_cast<T *>(m_logicMap.value(std::type_index(typeid(T))));
    }

private:
    QObject *m_qmlRoot{nullptr};

    QMap<std::type_index, PageLogicBase*> m_logicMap;

    QMap<Proto, PageProtocolLogicBase *> m_protocolLogicMap;

    VpnConnection* m_vpnConnection;
    QThread m_vpnConnectionThread;

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;
    std::shared_ptr<ServerController> m_serverController;

    NotificationHandler* m_notificationHandler;

    int m_selectedServerIndex = -1; // server index to use when proto settings page opened
    DockerContainer m_selectedDockerContainer; // same
    ServerCredentials m_installCredentials; // used to save cred between pages new_server and new_server_protocols and wizard
};
#endif // UILOGIC_H
