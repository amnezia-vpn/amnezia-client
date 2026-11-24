#include <QApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QUuid>

#include "testXrayPagesUi.h"
#include "ui/models/utils/jsonSyntaxHighlighter.h"
#include "ui/utils/pageEnum.h"
#include "vpnConnection.h"

using namespace amnezia;

void TestXrayPagesUi::initTestCase()
{
    step(QStringLiteral("create engine and core controller"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    PageLoader::declareQmlPageEnum();
    qmlRegisterType<JsonSyntaxHighlighter>("JsonSyntaxHighlighter", 1, 0, "JsonSyntaxHighlighter");
    for (const char *name : { "ProtocolEnum", "TransportProto", "ServiceType" })
        qmlRegisterUncreatableMetaObject(ProtocolEnumNS::staticMetaObject, "ProtocolEnum", 1, 0, name,
                                         QStringLiteral("Error: only enums"));

    m_settings =
            new SecureQSettings("AmneziaVPN-Test-" + QUuid::createUuid().toString(), "amnezia-client", nullptr, false);
    m_engine = new QQmlApplicationEngine(this);
    m_engine->addImportPath(QStringLiteral("qrc:/ui/qml/Modules/"));
    auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
    m_core = new TestCoreController(vpnConnection, m_settings, m_engine, this);
    m_model = qobject_cast<XraySettingsModel *>(
            m_engine->rootContext()->contextProperty(QStringLiteral("XraySettingsModel")).value<QObject *>());
    QVERIFY2(m_model, "XraySettingsModel is not exposed to QML");
    m_presets = qobject_cast<XrayPresetsModel *>(
            m_engine->rootContext()->contextProperty(QStringLiteral("XrayPresetsModel")).value<QObject *>());
    QVERIFY2(m_presets, "XrayPresetsModel is not exposed to QML");
    m_pages = m_core->pageController();
    QVERIFY(m_pages);

    step(QStringLiteral("add test server"));
    m_serverId = addServer(QStringLiteral("203.0.113.10"), QStringLiteral("not-used"),
                           serverConfigUtils::ConfigType::SelfHostedAdmin);
    QVERIFY2(!m_serverId.isEmpty(), "test server was not added");

    selectServer(m_serverId);
    QVERIFY2(m_core->m_serversUiController->isProcessedServerHasWriteAccess(), "test server must be editable");
}

void TestXrayPagesUi::init()
{
    step(QStringLiteral("init: clear custom presets, store a REALITY RAW Vision server and open its settings"));
    selectServer(m_serverId);
    clearCustomPresets();
    storeAndOpen(realityVision());
}

void TestXrayPagesUi::cleanup()
{
    closePage();
}

void TestXrayPagesUi::cleanupTestCase()
{
    closePage();
    clearCustomPresets();
    m_settings->clearSettings();
    delete m_core;
    delete m_engine;
    delete m_settings;
}

int main(int argc, char *argv[])
{
#ifndef Q_OS_MACOS
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
#endif
    QApplication app(argc, argv);
    TestXrayPagesUi test;
    QStringList args = app.arguments();
    if (!args.contains(QStringLiteral("-maxwarnings")))
        args << QStringLiteral("-maxwarnings") << QStringLiteral("0");
    return QTest::qExec(&test, args);
}
