#include <QJsonDocument>
#include <QRegularExpression>

#include "core/utils/errorCodes.h"
#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"

using namespace amnezia;

namespace
{
    const QString kDeviceOnlyText =
            QStringLiteral("The server will not be changed now. The new settings apply the next time you connect.");

    const QStringList kAllPages = { "PageProtocolXraySettings",          "PageProtocolXrayTransportSettings",
                                    "PageProtocolXraySecuritySettings",  "PageProtocolXrayFlowSettings",
                                    "PageProtocolXrayXPaddingSettings",  "PageProtocolXrayXPaddingBytesSettings",
                                    "PageProtocolXrayXmuxSettings" };
}

void TestXrayPagesUi::saveInitiallyDisabled_data()
{
    QTest::addColumn<QString>("page");
    for (const QString &p : kAllPages)
        QTest::newRow(qPrintable(p)) << p;
}

void TestXrayPagesUi::saveInitiallyDisabled()
{
    QFETCH(QString, page);

    loadPage(page);
    if (QTest::currentTestFailed())
        return;
    QQuickItem *save = saveButton();
    QVERIFY(save);
    QVERIFY(save->isVisible());
    QVERIFY(!save->isEnabled());

    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(save);
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 0);
}

void TestXrayPagesUi::saveEditThenRevert()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QSignalSpy unsaved(m_model, &XrayConfigModel::hasUnsavedChangesChanged);
    QQuickItem *fingerprint = dropDown(QStringLiteral("Fingerprint"));
    QVERIFY(chooseOption(fingerprint, QStringLiteral("firefox")));
    QVERIFY(saveButton()->isEnabled());
    QVERIFY(chooseOption(fingerprint, QStringLiteral("chrome")));
    QVERIFY(!saveButton()->isEnabled());
    QCOMPARE(unsaved.count(), 2);
}

void TestXrayPagesUi::saveReloadDiscardsEdits()
{
    storeAndOpen(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(QStringLiteral("Server Name (SNI)"));
    clearAndType(sni, QStringLiteral("vpn.example.org"));
    QVERIFY(m_model->hasUnsavedChanges());

    m_core->m_installUiController->openServerSettings(m_serverId, static_cast<int>(DockerContainer::Xray),
                                                      static_cast<int>(Proto::Xray));
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!saveButton()->isEnabled());
    QCOMPARE(m_model->sni(), QStringLiteral("www.example.com"));
    QCOMPARE(sni->property("text").toString(), QStringLiteral("www.example.com"));
}

void TestXrayPagesUi::saveCancelDoesNothing()
{
    storeAndOpen(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    m_model->setFingerprint(QStringLiteral("firefox"));
    QSignalSpy pages(m_pages, &PageController::goToPage);
    QSignalSpy finished(m_core->m_installUiController, &InstallUiController::updateContainerFinished);
    QSignalSpy failed(m_core->m_installUiController, &InstallUiController::installationErrorOccurred);
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
    QTest::qWait(300);

    QCOMPARE(pages.count(), 0);
    QCOMPARE(finished.count(), 0);
    QCOMPARE(failed.count(), 0);
    QVERIFY(m_model->hasUnsavedChanges());
    QVERIFY(saveButton()->isEnabled());
    QCOMPARE(stored().fingerprint, QStringLiteral("chrome"));
}

void TestXrayPagesUi::saveDeviceOnlyContinue_data()
{
    QTest::addColumn<QString>("page");
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("flow");
    QTest::addColumn<QString>("edit");
    QTest::newRow("security: fingerprint") << "PageProtocolXraySecuritySettings" << "raw" << "" << "fingerprint";
    QTest::newRow("flow: vision -> udp443") << "PageProtocolXrayFlowSettings" << "raw" << "xtls-rprx-vision" << "udp443";
    QTest::newRow("xmux: cMaxReuseTimes") << "PageProtocolXrayXmuxSettings" << "xhttp" << "" << "xmux";
    QTest::newRow("transport: uplink method") << "PageProtocolXrayTransportSettings" << "xhttp" << "" << "method";
}

void TestXrayPagesUi::saveDeviceOnlyContinue()
{
    QFETCH(QString, page);
    QFETCH(QString, transport);
    QFETCH(QString, flow);
    QFETCH(QString, edit);

    storeAndOpen(xraytest::baseConfig(transport, "reality", flow));
    loadPage(page);
    if (QTest::currentTestFailed())
        return;
    if (edit == QLatin1String("fingerprint"))
        m_model->setFingerprint(QStringLiteral("firefox"));
    else if (edit == QLatin1String("udp443"))
        m_model->setFlow(QStringLiteral("xtls-rprx-vision-udp443"));
    else if (edit == QLatin1String("xmux"))
        m_model->setXmuxCMaxReuseTimesMin(QStringLiteral("5"));
    else
        m_model->setXhttpUplinkMethod(QStringLiteral("PUT"));
    QVERIFY(!m_model->pendingChangeTouchesServer(QString()));

    QSignalSpy pages(m_pages, &PageController::goToPage);
    QSignalSpy finished(m_core->m_installUiController, &InstallUiController::updateContainerFinished);
    QSignalSpy failed(m_core->m_installUiController, &InstallUiController::installationErrorOccurred);
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(1).toString(), kDeviceOnlyText);
    answer(questions, true);

    QTRY_COMPARE(finished.count() + failed.count(), 1);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("Settings updated successfully"));
    QCOMPARE(finished.first().at(1).toBool(), false);
    QCOMPARE(pages.count(), 0);

    const XrayServerConfig after = stored();
    if (edit == QLatin1String("fingerprint"))
        QCOMPARE(after.fingerprint, QStringLiteral("firefox"));
    else if (edit == QLatin1String("udp443"))
        QCOMPARE(after.flow, QStringLiteral("xtls-rprx-vision-udp443"));
    else if (edit == QLatin1String("xmux"))
        QCOMPARE(after.xhttp.xmux.cMaxReuseTimesMin, QStringLiteral("5"));
    else
        QCOMPARE(after.xhttp.uplinkMethod, QStringLiteral("PUT"));
    QTRY_VERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::saveServerChangeOnHostlessServer()
{
    selectServer(m_hostlessServerId);
    storeAndOpen(xraytest::baseConfig("raw", "reality", ""), m_hostlessServerId);
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    m_model->setSni(QStringLiteral("www.example.org"));
    QVERIFY(m_model->pendingChangeTouchesServer(QString()));

    QSignalSpy pages(m_pages, &PageController::goToPage);
    QSignalSpy finished(m_core->m_installUiController, &InstallUiController::updateContainerFinished);
    QSignalSpy failed(m_core->m_installUiController, &InstallUiController::installationErrorOccurred);
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    answer(questions, true);

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)), PageLoader::PageEnum::PageSetupWizardInstalling);
    QTRY_COMPARE(finished.count() + failed.count(), 1);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(qvariant_cast<ErrorCode>(failed.first().at(0)), ErrorCode::InternalError);
    QCOMPARE(stored(m_hostlessServerId).sni, QStringLiteral("www.example.com"));
    QVERIFY(m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::saveCppGuardRefusesInvalid()
{
    m_model->setSni(QStringLiteral("*"));
    QSignalSpy failed(m_core->m_installUiController, &InstallUiController::installationErrorOccurred);
    m_core->m_installUiController->updateServerConfig(m_serverId, static_cast<int>(DockerContainer::Xray),
                                                      static_cast<int>(Proto::Xray), false);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(qvariant_cast<ErrorCode>(failed.first().at(0)), ErrorCode::XrayServerConfigInvalid);
    QCOMPARE(stored().sni, QStringLiteral("www.example.com"));
}

void TestXrayPagesUi::saveReadOnlySubpages_data()
{
    QTest::addColumn<QString>("page");
    for (const QString &p : kAllPages.mid(1))
        QTest::newRow(qPrintable(p)) << p;
}

void TestXrayPagesUi::saveReadOnlySubpages()
{
    QFETCH(QString, page);

    selectServer(m_readOnlyServerId);
    storeAndOpen(xraytest::baseConfig("xhttp", "reality", ""), m_readOnlyServerId);
    loadPage(page);
    if (QTest::currentTestFailed())
        return;

    QVERIFY(!saveButton()->isVisible());
    int controls = 0;
    for (const char *type : { "RadioItemType", "TextFieldType", "DropDownType", "SwitchRowType", "MinMaxRowType" }) {
        for (QQuickItem *control : allOfType(type)) {
            ++controls;
            QVERIFY2(!control->isEnabled(), qPrintable(QString::fromLatin1(type)));
        }
    }
    QVERIFY(controls > 0);
}

void TestXrayPagesUi::snapshotsEmptyState()
{
    XRAY_LOAD("PageProtocolXraySnapshots");
    QCOMPARE(m_snapshots->rowCount(), 0);
    QQuickItem *empty = typed("BodyTextType", "text",
                              QStringLiteral("No saved configurations yet.\nCreate one from the current settings."));
    QVERIFY(empty && empty->isVisible());
    QVERIFY(!typed("SmallTextType", "text", QStringLiteral("Configurations"))->isVisible());
}

void TestXrayPagesUi::snapshotsCreate()
{
    XRAY_LOAD("PageProtocolXraySnapshots");
    click(typed("ListItemType", "text", QStringLiteral("Create configuration based on current settings")));
    QCOMPARE(m_snapshots->rowCount(), 1);
    QQuickItem *row = nullptr;
    QTRY_VERIFY((row = typed("ListItemType", "text", QStringLiteral("XHTTP Reality"))) != nullptr);
    QVERIFY(QRegularExpression(QStringLiteral(R"(^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}$)"))
                    .match(row->property("subtitle").toString())
                    .hasMatch());
    QVERIFY(typed("SmallTextType", "text", QStringLiteral("Configurations"))->isVisible());
    QVERIFY(!m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::snapshotsApply()
{
    storeAndOpen(xraytest::baseConfig("raw", "tls", ""));
    m_snapshots->createFromCurrentModel();
    openStored(xraytest::baseConfig("xhttp", "reality", ""));
    XRAY_LOAD("PageProtocolXraySnapshots");

    QQuickItem *row = typed("ListItemType", "text", QStringLiteral("RAW (TCP) TLS"));
    QVERIFY(row);
    QObject *drawer = popupOf(m_page);
    QVERIFY(drawer);
    click(row);
    QVERIFY(QTest::qWaitFor([drawer] { return drawer->property("opened").toBool(); }, 3000));
    QTest::qWait(250);

    QSignalSpy closed(m_pages, &PageController::closePage);
    click(typed("ListItemType", "text", QStringLiteral("Apply configuration"), popupContent(drawer)));
    QCOMPARE(closed.count(), 1);
    QCOMPARE(m_model->transport(), QStringLiteral("raw"));
    QCOMPARE(m_model->security(), QStringLiteral("tls"));
    QVERIFY(m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::snapshotsDelete()
{
    m_snapshots->createFromCurrentModel();
    XRAY_LOAD("PageProtocolXraySnapshots");
    QSignalSpy removed(m_snapshots, &XrayConfigSnapshotsModel::configRemoved);
    QObject *drawer = popupOf(m_page);
    click(typed("ListItemType", "text", QStringLiteral("XHTTP Reality")));
    QVERIFY(QTest::qWaitFor([drawer] { return drawer->property("opened").toBool(); }, 3000));
    QTest::qWait(250);

    XRAY_QUESTIONS(questions);
    click(typed("ListItemType", "text", QStringLiteral("Delete configuration"), popupContent(drawer)));
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(0).toString(), QStringLiteral("Delete configuration?"));
    QCOMPARE(questions.first().at(1).toString(), QStringLiteral("This action cannot be undone."));
    QCOMPARE(questions.first().at(2).toString(), QStringLiteral("Delete"));
    QCOMPARE(questions.first().at(3).toString(), QStringLiteral("Cancel"));
    answer(questions, true);
    QCOMPARE(removed.count(), 1);
    QCOMPARE(m_snapshots->rowCount(), 0);
}

void TestXrayPagesUi::snapshotsDeleteCancelled()
{
    m_snapshots->createFromCurrentModel();
    XRAY_LOAD("PageProtocolXraySnapshots");
    QObject *drawer = popupOf(m_page);
    click(typed("ListItemType", "text", QStringLiteral("XHTTP Reality")));
    QVERIFY(QTest::qWaitFor([drawer] { return drawer->property("opened").toBool(); }, 3000));
    QTest::qWait(250);

    XRAY_QUESTIONS(questions);
    click(typed("ListItemType", "text", QStringLiteral("Delete configuration"), popupContent(drawer)));
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
    QCOMPARE(m_snapshots->rowCount(), 1);
}

void TestXrayPagesUi::snapshotsImportExport()
{
    m_snapshots->createFromCurrentModel();
    QCOMPARE(m_snapshots->rowCount(), 1);
    const QString json = m_snapshots->exportToJson(0);
    const QJsonObject exported = QJsonDocument::fromJson(json.toUtf8()).object();
    for (const char *key : { "id", "displayName", "createdAt", "serverConfig" })
        QVERIFY2(exported.contains(QLatin1String(key)), key);
    QCOMPARE(exported.value("displayName").toString(), QStringLiteral("XHTTP Reality"));

    QVERIFY(m_snapshots->importFromJson(json));
    QCOMPARE(m_snapshots->rowCount(), 2);

    XRAY_LOAD("PageProtocolXraySnapshots");
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);
    QSignalSpy importFailed(m_snapshots, &XrayConfigSnapshotsModel::importFailed);
    QVERIFY(!m_snapshots->importFromJson(QStringLiteral("[]")));
    QCOMPARE(importFailed.count(), 1);
    QCOMPARE(importFailed.first().at(0).toString(), QStringLiteral("Invalid JSON format"));
    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(), QStringLiteral("Invalid JSON format"));
    QCOMPARE(m_snapshots->rowCount(), 2);
}

void TestXrayPagesUi::snapshotsExportWithoutSnapshots()
{
    XRAY_LOAD("PageProtocolXraySnapshots");
    QSignalSpy busy(m_pages, &PageController::showBusyIndicator);
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);
    click(typed("ListItemType", "text", QStringLiteral("Export settings")));
    QTest::qWait(200);
    QCOMPARE(busy.count(), 0);
    QCOMPARE(notes.count(), 0);
}
