#include <QApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QUuid>

#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"
#include "vpnConnection.h"

using namespace amnezia;

void TestXrayPagesUi::initTestCase()
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    PageLoader::declareQmlPageEnum();
    for (const char *name : { "ProtocolEnum", "TransportProto", "ServiceType" })
        qmlRegisterUncreatableMetaObject(ProtocolEnumNS::staticMetaObject, "ProtocolEnum", 1, 0, name,
                                         QStringLiteral("Error: only enums"));

    m_settings = new SecureQSettings("AmneziaVPN-Test-" + QUuid::createUuid().toString(), "amnezia-client", nullptr, false);
    m_engine = new QQmlApplicationEngine(this);
    m_engine->addImportPath(QStringLiteral("qrc:/ui/qml/Modules/"));
    auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
    m_core = new TestCoreController(vpnConnection, m_settings, m_engine, this);
    m_model = qobject_cast<XrayConfigModel *>(
            m_engine->rootContext()->contextProperty(QStringLiteral("XrayConfigModel")).value<QObject *>());
    QVERIFY2(m_model, "XrayConfigModel is not exposed to QML");
    m_snapshots = qobject_cast<XrayConfigSnapshotsModel *>(
            m_engine->rootContext()->contextProperty(QStringLiteral("XrayConfigSnapshotsModel")).value<QObject *>());
    QVERIFY2(m_snapshots, "XrayConfigSnapshotsModel is not exposed to QML");
    m_pages = m_core->pageController();
    QVERIFY(m_pages);

    m_serverId = addServer(QStringLiteral("203.0.113.10"), QStringLiteral("not-used"),
                           serverConfigUtils::ConfigType::SelfHostedAdmin);
    m_readOnlyServerId = addServer(QStringLiteral("203.0.113.11"), QString(), serverConfigUtils::ConfigType::SelfHostedUser);
    m_hostlessServerId = addServer(QString(), QStringLiteral("not-used"), serverConfigUtils::ConfigType::SelfHostedAdmin);
    QVERIFY2(!m_serverId.isEmpty() && !m_readOnlyServerId.isEmpty() && !m_hostlessServerId.isEmpty(),
             "test servers were not added");

    selectServer(m_readOnlyServerId);
    QVERIFY2(!m_core->m_serversUiController->isProcessedServerHasWriteAccess(), "read-only server must not be editable");
    selectServer(m_hostlessServerId);
    QVERIFY2(m_core->m_serversUiController->isProcessedServerHasWriteAccess(), "hostless server must be editable");
    selectServer(m_serverId);
    QVERIFY2(m_core->m_serversUiController->isProcessedServerHasWriteAccess(), "test server must be editable");
}

void TestXrayPagesUi::init()
{
    selectServer(m_serverId);
    storeAndOpen(xraytest::baseConfig("xhttp", "reality", ""));
    m_core->m_appSettingsRepository->setXraySavedConfigs(QByteArray());
    m_snapshots->reload();
}

void TestXrayPagesUi::cleanup()
{
    closePage();
}

void TestXrayPagesUi::cleanupTestCase()
{
    closePage();
    m_settings->clearSettings();
    delete m_core;
    delete m_engine;
    delete m_settings;
}

void TestXrayPagesUi::transportRadioClickChangesModel()
{
    openStored(xraytest::baseConfig("xhttp", "reality", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");

    for (const auto &pair : { qMakePair(QStringLiteral("RAW (TCP)"), QStringLiteral("raw")),
                              qMakePair(QStringLiteral("mKCP"), QStringLiteral("mkcp")),
                              qMakePair(QStringLiteral("XHTTP"), QStringLiteral("xhttp")) }) {
        QQuickItem *radio = item("text", pair.first);
        QVERIFY(radio);
        click(radio);
        QCOMPARE(m_model->transport(), pair.second);
    }
}

void TestXrayPagesUi::packetUpFieldsFollowMode_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("security");
    QTest::addColumn<bool>("visible");
    QTest::newRow("stream-one + reality") << "Stream-one" << "reality" << false;
    QTest::newRow("stream-up + tls") << "Stream-up" << "tls" << false;
    QTest::newRow("packet-up + tls") << "Packet-up" << "tls" << true;
    QTest::newRow("auto + none") << "Auto" << "none" << true;
    QTest::newRow("auto + reality") << "Auto" << "reality" << false;
}

void TestXrayPagesUi::packetUpFieldsFollowMode()
{
    QFETCH(QString, mode);
    QFETCH(QString, security);
    QFETCH(bool, visible);

    XrayServerConfig srv = xraytest::baseConfig("xhttp", security, "");
    srv.xhttp.mode = mode;
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayTransportSettings");

    for (const QString &header : { QStringLiteral("SeqKey"), QStringLiteral("UplinkDataKey") }) {
        QQuickItem *f = item("headerText", header);
        QVERIFY(f);
        QCOMPARE(f->isVisible(), visible);
    }
    for (const QString &header : { QStringLiteral("SeqPlacement"), QStringLiteral("UplinkDataPlacement") }) {
        QQuickItem *dd = item("headerText", header);
        QVERIFY(dd);
        QCOMPARE(dd->isVisible(), visible);
    }
}

void TestXrayPagesUi::pathGetsLeadingSlash()
{
    openStored(xraytest::baseConfig("xhttp", "reality", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");

    QQuickItem *textField = field(QStringLiteral("Path"));
    QVERIFY(textField);
    clearAndType(textField, QStringLiteral("amnezia"));

    QCOMPARE(m_model->xhttpPath(), QStringLiteral("/amnezia"));
    QCOMPARE(textField->property("text").toString(), QStringLiteral("/amnezia"));
}

void TestXrayPagesUi::visionNeedsTlsOrReality_data()
{
    QTest::addColumn<QString>("security");
    QTest::addColumn<bool>("enabled");
    QTest::newRow("none") << "none" << false;
    QTest::newRow("tls") << "tls" << true;
    QTest::newRow("reality") << "reality" << true;
}

void TestXrayPagesUi::visionNeedsTlsOrReality()
{
    QFETCH(QString, security);
    QFETCH(bool, enabled);

    openStored(xraytest::baseConfig("raw", security, ""));
    XRAY_LOAD("PageProtocolXrayFlowSettings");

    for (const QString &flow : { QStringLiteral("xtls-rprx-vision"), QStringLiteral("xtls-rprx-vision-udp443") }) {
        QQuickItem *radio = item("text", flow);
        QVERIFY(radio);
        QCOMPARE(radio->isEnabled(), enabled);
    }
}

void TestXrayPagesUi::fingerprintHiddenOnMkcp_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<bool>("visible");
    QTest::newRow("raw + tls") << "raw" << true;
    QTest::newRow("mkcp + tls") << "mkcp" << false;
}

void TestXrayPagesUi::fingerprintHiddenOnMkcp()
{
    QFETCH(QString, transport);
    QFETCH(bool, visible);

    openStored(xraytest::baseConfig(transport, "tls", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");

    QQuickItem *dd = item("descriptionText", QStringLiteral("Fingerprint"));
    QVERIFY(dd);
    QCOMPARE(dd->isVisible(), visible);
}

void TestXrayPagesUi::sniRejectsWildcardInput()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");

    QQuickItem *textField = visibleField(QStringLiteral("Server Name (SNI)"));
    QVERIFY(textField);
    clearAndType(textField, QStringLiteral("*.example.com"));

    QVERIFY2(!textField->property("text").toString().contains(QLatin1Char('*')),
             qPrintable(textField->property("text").toString()));
    QVERIFY(!m_model->sni().contains(QLatin1Char('*')));
}

void TestXrayPagesUi::xPaddingFieldsFollowObfsMode_data()
{
    QTest::addColumn<bool>("obfs");
    QTest::newRow("obfs on") << true;
    QTest::newRow("obfs off") << false;
}

void TestXrayPagesUi::xPaddingFieldsFollowObfsMode()
{
    QFETCH(bool, obfs);

    XrayServerConfig srv = xraytest::baseConfig("xhttp", "reality", "");
    srv.xhttp.xPadding.obfsMode = obfs;
    srv.xhttp.xPadding.bytesMin.clear();
    srv.xhttp.xPadding.bytesMax.clear();
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayXPaddingSettings");

    QQuickItem *bytes = item("text", QStringLiteral("xPaddingBytes"));
    QVERIFY(bytes);
    QCOMPARE(bytes->isEnabled(), obfs);
    QCOMPARE(bytes->property("subtitle").toString(), QStringLiteral("Default"));

    for (const QString &header : { QStringLiteral("xPaddingKey"), QStringLiteral("xPaddingHeader"),
                                   QStringLiteral("xPaddingPlacement"), QStringLiteral("xPaddingMethod") }) {
        QQuickItem *f = item("headerText", header);
        QVERIFY(f);
        QCOMPARE(f->isEnabled(), obfs);
    }
}

void TestXrayPagesUi::subpageSaveRefusesInvalidSni_data()
{
    QTest::addColumn<QString>("page");
    QTest::newRow("flow") << "PageProtocolXrayFlowSettings";
    QTest::newRow("xmux") << "PageProtocolXrayXmuxSettings";
    QTest::newRow("xPadding") << "PageProtocolXrayXPaddingSettings";
    QTest::newRow("xPaddingBytes") << "PageProtocolXrayXPaddingBytesSettings";
    QTest::newRow("security") << "PageProtocolXraySecuritySettings";
    QTest::newRow("transport") << "PageProtocolXrayTransportSettings";
}

void TestXrayPagesUi::subpageSaveRefusesInvalidSni()
{
    QFETCH(QString, page);

    openStored(xraytest::baseConfig("xhttp", "reality", ""));
    m_model->setSni(QStringLiteral("*"));
    QVERIFY(m_model->hasUnsavedChanges());
    loadPage(page);
    if (QTest::currentTestFailed())
        return;

    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    QQuickItem *save = saveButton();
    QVERIFY(save);
    QVERIFY2(save->isVisible() && save->isEnabled(), "Save must be shown for an edited, writable server");
    click(save);

    QCOMPARE(errors.count(), 1);
    QVERIFY2(errors.first().first().toString().contains(QLatin1String("SNI")),
             qPrintable(errors.first().first().toString()));
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
