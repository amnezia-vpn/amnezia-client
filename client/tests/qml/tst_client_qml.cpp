#include <QtQuickTest/quicktest.h>

#include <QQmlContext>
#include <QQmlEngine>
#include <QObject>

namespace TestPageEnumNS
{
Q_NAMESPACE

enum PageEnum {
    PageDeinstalling = 0,
    PageSetupWizardEasy
};
Q_ENUM_NS(PageEnum)
} // namespace TestPageEnumNS

class MockSettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int safeAreaTopMargin READ safeAreaTopMargin CONSTANT)

public:
    int safeAreaTopMargin() const { return 0; }

    Q_INVOKABLE bool isAmneziaDnsEnabled() const { return false; }
};

class MockContainersModel : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE QString getProcessedContainerName() const { return QStringLiteral("AmneziaDNS"); }
};

class MockFocusController : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void resetRootObject() {}
    Q_INVOKABLE void setFocusOnDefaultItem() {}
    Q_INVOKABLE void nextKeyTabItem() {}
    Q_INVOKABLE void previousKeyTabItem() {}
    Q_INVOKABLE void nextKeyUpItem() {}
    Q_INVOKABLE void nextKeyDownItem() {}
    Q_INVOKABLE void nextKeyLeftItem() {}
    Q_INVOKABLE void nextKeyRightItem() {}
};

class MockLanguageModel : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE int getLineHeightAppend() const { return 0; }
};

class MockServersModel : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE bool isDefaultServerCurrentlyProcessed() const { return false; }
};

class MockConnectionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected CONSTANT)

public:
    bool isConnected() const { return false; }
};

class MockPageController : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void showNotificationMessage(const QString &) {}
    Q_INVOKABLE void goToPage(int, bool = true) {}
};

class MockInstallController : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void removeProcessedContainer() {}
};

class ClientQmlSetup : public QObject
{
    Q_OBJECT

public slots:
    void applicationAvailable()
    {
        qmlRegisterUncreatableMetaObject(TestPageEnumNS::staticMetaObject, "PageEnum", 1, 0, "PageEnum", "Error: only enums");
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->addImportPath(QStringLiteral(CLIENT_QML_MODULES_DIR));
        engine->rootContext()->setContextProperty("SettingsController", &m_settingsController);
        engine->rootContext()->setContextProperty("ContainersModel", &m_containersModel);
        engine->rootContext()->setContextProperty("FocusController", &m_focusController);
        engine->rootContext()->setContextProperty("LanguageModel", &m_languageModel);
        engine->rootContext()->setContextProperty("ServersModel", &m_serversModel);
        engine->rootContext()->setContextProperty("ConnectionController", &m_connectionController);
        engine->rootContext()->setContextProperty("PageController", &m_pageController);
        engine->rootContext()->setContextProperty("InstallController", &m_installController);
    }

private:
    MockSettingsController m_settingsController;
    MockContainersModel m_containersModel;
    MockFocusController m_focusController;
    MockLanguageModel m_languageModel;
    MockServersModel m_serversModel;
    MockConnectionController m_connectionController;
    MockPageController m_pageController;
    MockInstallController m_installController;
};

QUICK_TEST_MAIN_WITH_SETUP(ClientQml, ClientQmlSetup)

#include "tst_client_qml.moc"
