#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"

using namespace amnezia;

namespace
{
    const QString kPortChangeText = QStringLiteral(
            "All users with whom you shared a connection with will no longer be able to connect to it. "
            "You will need to share the connection again.");
}

void TestXrayPagesUi::mainPortPlaceholderAndDefault()
{
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setPort(QString());
    QQuickItem *port = field(QStringLiteral("Port"));
    QVERIFY(port);
    QCOMPARE(port->property("text").toString(), QString());
    QCOMPARE(port->property("placeholderText").toString(), QStringLiteral("443"));
}

void TestXrayPagesUi::mainPortInput_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("expected");
    QTest::newRow("letters and symbols are blocked") << "a-+ .x1" << "1";
    QTest::newRow("65536 cannot be typed") << "65536" << "6553";
    QTest::newRow("70000 cannot be typed") << "70000" << "7000";
    QTest::newRow("0 is clamped to 1") << "0" << "1";
    QTest::newRow("0000 is clamped to 1") << "0000" << "1";
    QTest::newRow("leading zeros dropped") << "0443" << "443";
    QTest::newRow("65535 accepted") << "65535" << "65535";
}

void TestXrayPagesUi::mainPortInput()
{
    QFETCH(QString, typed);
    QFETCH(QString, expected);

    XRAY_LOAD("PageProtocolXraySettings");
    QQuickItem *port = field(QStringLiteral("Port"));
    QVERIFY(port);
    clearAndType(port, typed);

    QCOMPARE(port->property("text").toString(), expected);
    QCOMPARE(m_model->port(), expected);
}

void TestXrayPagesUi::mainPortEditTogglesSave()
{
    XRAY_LOAD("PageProtocolXraySettings");
    QQuickItem *port = field(QStringLiteral("Port"));
    QQuickItem *save = saveButton();
    QVERIFY(port && save);
    QVERIFY(!save->isEnabled());

    clearAndType(port, QStringLiteral("8443"), false);
    QCOMPARE(m_model->port(), QStringLiteral("8443"));
    QVERIFY(m_model->hasUnsavedChanges());
    QVERIFY(save->isEnabled());

    clearAndType(port, QStringLiteral("443"), false);
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!save->isEnabled());
}

void TestXrayPagesUi::mainEmptyPortDisablesSave()
{
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setXhttpDisableGrpc(false);
    QVERIFY(m_model->hasUnsavedChanges());
    QQuickItem *port = field(QStringLiteral("Port"));
    QQuickItem *save = saveButton();
    QVERIFY(port && save);
    QVERIFY(save->isEnabled());

    clearAndType(port, QString(), false);
    QCOMPARE(m_model->port(), QString());
    QVERIFY2(!save->isEnabled(), "Save must stay disabled while the port is empty");
}

void TestXrayPagesUi::mainEmptyPortErrorOnFocusOut()
{
    XRAY_LOAD("PageProtocolXraySettings");
    QQuickItem *portField = typed("TextFieldType", "headerText", QStringLiteral("Port"));
    QQuickItem *port = input(portField);
    QVERIFY(port);

    clearAndType(port, QString(), false);
    port->setProperty("focus", false);
    QTest::qWait(50);
    QCOMPARE(portField->property("errorText").toString(), QStringLiteral("The field can't be empty"));

    click(port);
    type(QStringLiteral("1"));
    QCOMPARE(portField->property("errorText").toString(), QString());
}

void TestXrayPagesUi::mainPortFollowsModelOnlyWhenUnfocused()
{
    XRAY_LOAD("PageProtocolXraySettings");
    QQuickItem *port = field(QStringLiteral("Port"));
    QVERIFY(port);

    m_model->setPort(QStringLiteral("2053"));
    QCOMPARE(port->property("text").toString(), QStringLiteral("2053"));

    click(port);
    m_model->setPort(QStringLiteral("9999"));
    QCOMPARE(port->property("text").toString(), QStringLiteral("2053"));
}

void TestXrayPagesUi::mainSaveRefusesInvalidPort()
{
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setPort(QStringLiteral("0"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    QQuickItem *save = saveButton();
    QVERIFY(save && save->isEnabled());
    click(save);

    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().first().toString(), QStringLiteral("Port must be in the range of 1 to 65535"));
}

void TestXrayPagesUi::mainSaveJoinsErrors()
{
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setPort(QStringLiteral("0"));
    m_model->setSni(QStringLiteral("*"));
    m_model->setXhttpHost(QStringLiteral("bad host"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    click(saveButton());

    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().first().toString(),
             QStringLiteral("Port must be in the range of 1 to 65535\n"
                            "SNI: enter a valid IP address or domain name\n"
                            "Host: enter a valid IP address or domain name"));
}

void TestXrayPagesUi::mainPortChangeDialog()
{
    XRAY_LOAD("PageProtocolXraySettings");
    XRAY_QUESTIONS(questions);
    clearAndType(field(QStringLiteral("Port")), QStringLiteral("8443"));
    click(saveButton());

    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(0).toString(), QStringLiteral("Save settings?"));
    QCOMPARE(questions.first().at(1).toString(), kPortChangeText);
    QCOMPARE(questions.first().at(2).toString(), QStringLiteral("Continue"));
    QCOMPARE(questions.first().at(3).toString(), QStringLiteral("Cancel"));
    answer(questions, false);
}

void TestXrayPagesUi::mainTransportSubtitle_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("subtitle");
    QTest::newRow("raw") << "raw" << "RAW (TCP)";
    QTest::newRow("xhttp") << "xhttp" << "XHTTP";
    QTest::newRow("mkcp") << "mkcp" << "mKCP";
}

void TestXrayPagesUi::mainTransportSubtitle()
{
    QFETCH(QString, transport);
    QFETCH(QString, subtitle);

    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setTransport(transport);
    QQuickItem *row = typed("ListItemType", "text", QStringLiteral("Transport"));
    QVERIFY(row);
    QCOMPARE(row->property("subtitle").toString(), subtitle);
}

void TestXrayPagesUi::mainSecuritySubtitle_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("subtitle");
    QTest::newRow("raw + none") << "raw" << "none" << "None";
    QTest::newRow("raw + tls") << "raw" << "tls" << "TLS";
    QTest::newRow("raw + reality") << "raw" << "reality" << "Reality";
    QTest::newRow("mkcp + tls") << "mkcp" << "tls" << "TLS";
    QTest::newRow("mkcp + reality shows None") << "mkcp" << "reality" << "None";
}

void TestXrayPagesUi::mainSecuritySubtitle()
{
    QFETCH(QString, transport);
    QFETCH(QString, security);
    QFETCH(QString, subtitle);

    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setTransport(transport);
    m_model->setSecurity(security);
    QQuickItem *row = typed("ListItemType", "text", QStringLiteral("Security"));
    QVERIFY(row);
    QCOMPARE(row->property("subtitle").toString(), subtitle);
}

void TestXrayPagesUi::mainFlowSubtitle_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("flow");
    QTest::addColumn<QString>("subtitle");
    QTest::newRow("raw + tls + vision") << "raw" << "tls" << "xtls-rprx-vision" << "xtls-rprx-vision";
    QTest::newRow("raw + reality + udp443") << "raw" << "reality" << "xtls-rprx-vision-udp443" << "xtls-rprx-vision-udp443";
    QTest::newRow("raw + reality + empty") << "raw" << "reality" << "" << "Empty";
    QTest::newRow("raw + none + vision") << "raw" << "none" << "xtls-rprx-vision" << "Empty";
    QTest::newRow("xhttp + reality + vision") << "xhttp" << "reality" << "xtls-rprx-vision" << "Empty";
    QTest::newRow("mkcp + tls + vision") << "mkcp" << "tls" << "xtls-rprx-vision" << "Empty";
}

void TestXrayPagesUi::mainFlowSubtitle()
{
    QFETCH(QString, transport);
    QFETCH(QString, security);
    QFETCH(QString, flow);
    QFETCH(QString, subtitle);

    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setTransport(transport);
    m_model->setSecurity(security);
    m_model->setFlow(flow);
    QQuickItem *row = typed("ListItemType", "text", QStringLiteral("Flow"));
    QVERIFY(row);
    QCOMPARE(row->property("subtitle").toString(), subtitle);
}

void TestXrayPagesUi::mainRowsNavigate_data()
{
    QTest::addColumn<QString>("row");
    QTest::addColumn<int>("page");
    QTest::newRow("Transport") << "Transport" << static_cast<int>(PageLoader::PageEnum::PageProtocolXrayTransportSettings);
    QTest::newRow("Security") << "Security" << static_cast<int>(PageLoader::PageEnum::PageProtocolXraySecuritySettings);
    QTest::newRow("Flow") << "Flow" << static_cast<int>(PageLoader::PageEnum::PageProtocolXrayFlowSettings);
}

void TestXrayPagesUi::mainRowsNavigate()
{
    QFETCH(QString, row);
    QFETCH(int, page);

    XRAY_LOAD("PageProtocolXraySettings");
    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(typed("ListItemType", "text", row));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(static_cast<int>(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0))), page);
}

void TestXrayPagesUi::mainSnapshotsIconNavigates()
{
    XRAY_LOAD("PageProtocolXraySettings");
    QSignalSpy pages(m_pages, &PageController::goToPage);
    QQuickItem *icon = iconButton(QStringLiteral("more-vertical.svg"));
    QVERIFY(icon);
    click(icon);

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)), PageLoader::PageEnum::PageProtocolXraySnapshots);
}

void TestXrayPagesUi::mainBackButtonClosesPage()
{
    XRAY_LOAD("PageProtocolXraySettings");
    QSignalSpy closed(m_pages, &PageController::closePage);
    QQuickItem *back = iconButton(QStringLiteral("arrow-left.svg"));
    QVERIFY(back);
    click(back);
    QCOMPARE(closed.count(), 1);
}

void TestXrayPagesUi::mainResetDialogTexts()
{
    XRAY_LOAD("PageProtocolXraySettings");
    XRAY_QUESTIONS(questions);
    click(typed("ListItemType", "text", QStringLiteral("Reset settings")));

    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(0).toString(), QStringLiteral("Reset settings?"));
    QCOMPARE(questions.first().at(1).toString(), QStringLiteral("All XRay settings will be restored to defaults."));
    QCOMPARE(questions.first().at(2).toString(), QStringLiteral("Reset"));
    QCOMPARE(questions.first().at(3).toString(), QStringLiteral("Cancel"));
}

void TestXrayPagesUi::mainResetAccepted()
{
    XrayServerConfig srv = xraytest::baseConfig("xhttp", "tls", "");
    srv.port = QStringLiteral("8443");
    storeAndOpen(srv);
    XRAY_LOAD("PageProtocolXraySettings");
    XRAY_QUESTIONS(questions);
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);
    click(typed("ListItemType", "text", QStringLiteral("Reset settings")));
    QCOMPARE(questions.count(), 1);
    answer(questions, true);

    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(),
             QStringLiteral("Settings were reset to defaults. Tap Save to apply them on the server."));
    QCOMPARE(m_model->transport(), QStringLiteral("raw"));
    QCOMPARE(m_model->security(), QStringLiteral("reality"));
    QCOMPARE(m_model->flow(), QStringLiteral("xtls-rprx-vision"));
    QCOMPARE(m_model->port(), QStringLiteral("443"));
    QCOMPARE(m_model->sni(), QStringLiteral("www.googletagmanager.com"));
    QCOMPARE(typed("ListItemType", "text", QStringLiteral("Transport"))->property("subtitle").toString(),
             QStringLiteral("RAW (TCP)"));
    QCOMPARE(typed("ListItemType", "text", QStringLiteral("Security"))->property("subtitle").toString(),
             QStringLiteral("Reality"));
    QCOMPARE(typed("ListItemType", "text", QStringLiteral("Flow"))->property("subtitle").toString(),
             QStringLiteral("xtls-rprx-vision"));
    QCOMPARE(field(QStringLiteral("Port"))->property("text").toString(), QStringLiteral("443"));
    QVERIFY(saveButton()->isEnabled());
}

void TestXrayPagesUi::mainResetCancelled()
{
    XRAY_LOAD("PageProtocolXraySettings");
    XRAY_QUESTIONS(questions);
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);
    click(typed("ListItemType", "text", QStringLiteral("Reset settings")));
    QCOMPARE(questions.count(), 1);
    answer(questions, false);

    QCOMPARE(notes.count(), 0);
    QCOMPARE(m_model->transport(), QStringLiteral("xhttp"));
    QVERIFY(!m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::mainKeyMigrationDialog()
{
    XRAY_LOAD("PageProtocolXraySettings");
    XRAY_QUESTIONS(questions);
    QSignalSpy busy(m_pages, &PageController::showBusyIndicator);
    emit m_core->m_installUiController->xrayKeyMigrationNeedsConfirm(QStringLiteral("MSG"));
    QTest::qWait(50);

    QCOMPARE(busy.count(), 1);
    QCOMPARE(busy.first().at(0).toBool(), false);
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(0).toString(), QStringLiteral("Recreate XRay with new keys?"));
    QCOMPARE(questions.first().at(1).toString(), QStringLiteral("MSG"));
    QCOMPARE(questions.first().at(2).toString(), QStringLiteral("Continue"));
    QCOMPARE(questions.first().at(3).toString(), QStringLiteral("Cancel"));
    answer(questions, false);
}

void TestXrayPagesUi::mainReadOnlyServer()
{
    selectServer(m_readOnlyServerId);
    storeAndOpen(xraytest::baseConfig("xhttp", "reality", ""), m_readOnlyServerId);
    XRAY_LOAD("PageProtocolXraySettings");
    QSignalSpy pages(m_pages, &PageController::goToPage);

    QQuickItem *note = typed("SmallTextType", "text",
                             QStringLiteral("You have read-only access to this server. XRay settings cannot be edited."));
    QVERIFY(note && note->isVisible());
    QVERIFY(!typed("ListItemType", "text", QStringLiteral("Reset settings"))->isVisible());
    QVERIFY(!saveButton()->isVisible());
    QVERIFY(!field(QStringLiteral("Port"))->isEnabled());
    for (const QString &row : { QStringLiteral("Transport"), QStringLiteral("Security"), QStringLiteral("Flow") })
        QVERIFY2(!typed("ListItemType", "text", row)->isEnabled(), qPrintable(row));

    click(typed("ListItemType", "text", QStringLiteral("Transport")));
    QCOMPARE(pages.count(), 0);
}

void TestXrayPagesUi::mainWritableServerBaseline()
{
    XRAY_LOAD("PageProtocolXraySettings");
    QQuickItem *note = typed("SmallTextType", "text",
                             QStringLiteral("You have read-only access to this server. XRay settings cannot be edited."));
    QVERIFY(note && !note->isVisible());
    QVERIFY(typed("ListItemType", "text", QStringLiteral("Reset settings"))->isVisible());
    QQuickItem *save = saveButton();
    QVERIFY(save->isVisible());
    QVERIFY(!save->isEnabled());
}
