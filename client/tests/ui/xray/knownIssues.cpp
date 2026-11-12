#include "testXrayPagesUi.h"

using namespace amnezia;

void TestXrayPagesUi::issueTransportRoundTripLeavesSaveEnabled()
{
    storeAndOpen(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    click(typed("RadioItemType", "text", QStringLiteral("XHTTP")));
    click(typed("RadioItemType", "text", QStringLiteral("RAW (TCP)")));
    QCOMPARE(m_model->transport(), QStringLiteral("raw"));

    QVERIFY2(!m_model->hasUnsavedChanges(), "raw -> XHTTP -> raw left the model dirty");
    QVERIFY2(!saveButton()->isEnabled(), "Save is enabled although nothing was changed");
}

void TestXrayPagesUi::issueSubpageSavesEmptyPort()
{
    XrayServerConfig srv = xraytest::baseConfig("xhttp", "reality", "");
    srv.port = QStringLiteral("8443");
    storeAndOpen(srv);
    m_model->setPort(QString());
    m_model->setXhttpUplinkMethod(QStringLiteral("PUT"));
    XRAY_LOAD("PageProtocolXrayTransportSettings");

    XRAY_QUESTIONS(questions);
    QQuickItem *save = saveButton();
    if (save->isEnabled())
        click(save);
    const bool proceeded = questions.count() > 0;
    if (proceeded)
        answer(questions, false);
    QVERIFY2(!proceeded, "a sub-page offered to save with an empty port");
}

void TestXrayPagesUi::issueHiddenRealitySniBlocksMkcpSave()
{
    XrayServerConfig srv = xraytest::baseConfig("raw", "reality", "");
    srv.sni = QStringLiteral("*.asd.com");
    srv.site = srv.sni;
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    click(typed("RadioItemType", "text", QStringLiteral("mKCP")));

    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    if (questions.count() > 0)
        answer(questions, false);
    QVERIFY2(errors.isEmpty(), qPrintable(QStringLiteral("refused with: ") + (errors.isEmpty() ? QString()
                                                                              : errors.first().first().toString())));
}

void TestXrayPagesUi::issueClearingXmuxZeroEnablesSave()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(minMaxInput(minMaxRow(0), QStringLiteral("Min")), QString());
    QCOMPARE(m_model->xmuxMaxConcurrencyMin(), QString());
    QVERIFY2(!m_model->hasUnsavedChanges(), "clearing a 0 field counts as an unsaved change");
    QVERIFY2(!saveButton()->isEnabled(), "Save is enabled for a no-op");
}

void TestXrayPagesUi::issueSnapshotWithoutPortApplyIsSilent()
{
    const QString snapshot = QStringLiteral(
            R"({"displayName":"no port","serverConfig":{"xray_transport":"raw","xray_security":"tls"}})");
    QVERIFY(m_snapshots->importFromJson(snapshot));
    XRAY_LOAD("PageProtocolXraySnapshots");
    QCOMPARE(m_model->transport(), QStringLiteral("xhttp"));

    QObject *drawer = popupOf(m_page);
    click(typed("ListItemType", "text", QStringLiteral("no port")));
    QVERIFY(QTest::qWaitFor([drawer] { return drawer->property("opened").toBool(); }, 3000));
    QTest::qWait(250);

    const QString portBefore = m_model->port();
    click(typed("ListItemType", "text", QStringLiteral("Apply configuration"), popupContent(drawer)));
    QCOMPARE(m_model->transport(), QStringLiteral("raw"));
    QCOMPARE(m_model->security(), QStringLiteral("tls"));
    QCOMPARE(m_model->port(), portBefore);
}

void TestXrayPagesUi::issueSaveWithoutReturnSkipsClamp()
{
    openStored(xraytest::baseConfig("mkcp", "none", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    clearAndType(field(QStringLiteral("TTI")), QStringLiteral("5"), false);
    QCOMPARE(m_model->mkcpTti(), QStringLiteral("5"));

    XRAY_QUESTIONS(questions);
    click(saveButton());
    if (questions.count() > 0)
        answer(questions, false);
    QCOMPARE(m_model->mkcpTti(), QStringLiteral("10"));
}

void TestXrayPagesUi::issueSaveWithoutReturnKeepsRawKeepAlive()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(field(QStringLiteral("hKeepAlivePeriod")), QStringLiteral("-"), false);
    QCOMPARE(m_model->xmuxHKeepAlivePeriod(), QStringLiteral("-"));

    XRAY_QUESTIONS(questions);
    click(saveButton());
    if (questions.count() > 0)
        answer(questions, false);
    QCOMPARE(m_model->xmuxHKeepAlivePeriod(), QString());
}
