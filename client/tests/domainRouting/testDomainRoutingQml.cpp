#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStringListModel>
#include <QTest>

class TestDomainRoutingQml : public QObject
{
    Q_OBJECT

private slots:
    /// <summary>Проверяет загрузку страницы, переключатель доменов и ввод маски.</summary>
    void loadPage();
};

void TestDomainRoutingQml::loadPage()
{
    qmlRegisterModule("PageEnum", 1, 0);
    qmlRegisterModule("ContainerProps", 1, 0);
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(AMNEZIA_QML_ROOT "/Modules"));
    QStringList errors;
    connect(&engine, &QQmlEngine::warnings, &engine, [&errors](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) {
            errors.append(warning.toString());
        }
    });

    QQmlComponent fixture(&engine);
    fixture.setData(R"(
import QtQuick
QtObject {
    property int routeMode: 2
    property bool isSplitTunnelingEnabled: true
    property bool dynamicDomainRoutingEnabled: false
    property bool isConnected: false
    property bool isDefaultServerDefaultContainerHasSplitTunneling: false
    property string defaultServerId: "test"
    property int safeAreaTopMargin: 0
    property int safeAreaBottomMargin: 0
    property int imeHeight: 0
    signal finished(string message)
    signal errorOccurred(string errorMessage)
    function isServerFromApi(id) { return false }
    function getLineHeightAppend() { return 0 }
})",
                    QUrl());
    QScopedPointer<QObject> controller(fixture.create());
    QVERIFY2(controller, qPrintable(fixture.errorString()));
    for (const QString &name : { QStringLiteral("IpSplitTunnelingController"), QStringLiteral("ServersUiController"),
                                 QStringLiteral("ConnectionController"), QStringLiteral("PageController"),
                                 QStringLiteral("LanguageUiController") }) {
        engine.rootContext()->setContextProperty(name, controller.data());
    }
    QStringListModel model;
    engine.rootContext()->setContextProperty(QStringLiteral("IpSplitTunnelingModel"), &model);
    QQmlComponent component(
            &engine, QUrl::fromLocalFile(QStringLiteral(AMNEZIA_QML_ROOT "/Pages2/PageSettingsSplitTunneling.qml")));
    QTRY_VERIFY(component.status() != QQmlComponent::Loading);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> page(component.createWithInitialProperties({ { "width", 420 }, { "height", 800 } }));
    QVERIFY2(page, qPrintable(component.errorString()));
    QQuickWindow window;
    qobject_cast<QQuickItem *>(page.data())->setParentItem(window.contentItem());
    QCoreApplication::processEvents();
    QObject *toggle = page->findChild<QObject *>(QStringLiteral("domainRoutingSwitch"));
    QVERIFY(toggle);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(controller->setProperty("dynamicDomainRoutingEnabled", true));
    QTRY_VERIFY(toggle->property("checked").toBool());
    QVERIFY(toggle->setProperty("checked", false));
    QVERIFY(QMetaObject::invokeMethod(toggle, "toggled"));
    QVERIFY(!controller->property("dynamicDomainRoutingEnabled").toBool());
    QObject *panel = page->findChild<QObject *>(QStringLiteral("splitTunnelingAddSitePanel"));
    QVERIFY(panel);
    QObject *field = panel->property("textField").value<QObject *>();
    QVERIFY(field);
    QVERIFY(field->setProperty("text", QStringLiteral("*.example.com")));
    QCoreApplication::processEvents();
    QCOMPARE(page->property("searchPattern").toString(), QStringLiteral(".*\\*\\.example\\.com.*"));
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
}

QTEST_MAIN(TestDomainRoutingQml)
#include "testDomainRoutingQml.moc"
