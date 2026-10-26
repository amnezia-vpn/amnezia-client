#include "testXrayPagesUi.h"

using namespace amnezia;

namespace
{
    const QString kXmuxRule = QStringLiteral("Set either maxConcurrency or maxConnections, not both");
    const QString kXmuxError = QStringLiteral("XMux: set either maxConcurrency or maxConnections, not both");
    const QString kDeviceOnlyText =
            QStringLiteral("The server will not be changed now. The new settings apply the next time you connect.");
}

void TestXrayPagesUi::xmuxInitialState()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    QQuickItem *row = typed("SwitchRowType", "descriptionText", kXmuxRule);
    QVERIFY(row);
    QVERIFY(switcherOf(row)->property("checked").toBool());
    QVERIFY(typed("SmallTextType", "text", kXmuxRule)->isVisible());

    const QList<QQuickItem *> rows = allOfType("MinMaxRowType");
    QCOMPARE(rows.size(), 5);
    for (QQuickItem *r : rows) {
        for (const QString which : { QStringLiteral("Min"), QStringLiteral("Max") }) {
            QQuickItem *in = minMaxInput(r, which);
            QVERIFY(in);
            QCOMPARE(in->property("text").toString(), QStringLiteral("0"));
            QCOMPARE(in->property("placeholderText").toString(), QStringLiteral("0"));
        }
    }
    QQuickItem *keepAlive = typed("TextFieldType", "headerText", QStringLiteral("hKeepAlivePeriod"));
    QCOMPARE(input(keepAlive)->property("text").toString(), QString());
    QCOMPARE(keepAlive->property("helperText").toString(), QStringLiteral("HTTP keep-alive period. Integer, may be negative."));
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::xmuxSwitchDisablesFields()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    QQuickItem *sw = switcherOf(typed("SwitchRowType", "descriptionText", kXmuxRule));
    QVERIFY(sw);
    click(sw);
    QCOMPARE(m_model->xmuxEnabled(), false);
    for (QQuickItem *r : allOfType("MinMaxRowType")) {
        QVERIFY(!minMaxInput(r, QStringLiteral("Min"))->isEnabled());
        QVERIFY(!minMaxInput(r, QStringLiteral("Max"))->isEnabled());
    }
    QVERIFY(!field(QStringLiteral("hKeepAlivePeriod"))->isEnabled());
    QVERIFY(saveButton()->isEnabled());

    click(sw);
    QCOMPARE(m_model->xmuxEnabled(), true);
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::xmuxMinRaisesMax()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(minMaxInput(minMaxRow(0), QStringLiteral("Min")), QStringLiteral("16"));
    QCOMPARE(m_model->xmuxMaxConcurrencyMin(), QStringLiteral("16"));
    QCOMPARE(m_model->xmuxMaxConcurrencyMax(), QStringLiteral("16"));
}

void TestXrayPagesUi::xmuxMaxBelowMin()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    m_model->setXmuxMaxConnectionsMin(QStringLiteral("4"));
    m_model->setXmuxMaxConnectionsMax(QStringLiteral("4"));
    clearAndType(minMaxInput(minMaxRow(1), QStringLiteral("Max")), QStringLiteral("2"));
    QCOMPARE(m_model->xmuxMaxConnectionsMax(), QStringLiteral("4"));
}

void TestXrayPagesUi::xmuxBothLimitsRefusedOnSave()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(minMaxInput(minMaxRow(0), QStringLiteral("Min")), QStringLiteral("16"));
    clearAndType(minMaxInput(minMaxRow(1), QStringLiteral("Min")), QStringLiteral("4"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    click(saveButton());
    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().first().toString(), kXmuxError);
}

void TestXrayPagesUi::xmuxZeroCountsAsUnset()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(minMaxInput(minMaxRow(0), QStringLiteral("Min")), QStringLiteral("16"));
    QCOMPARE(m_model->xmuxMaxConnectionsMin(), QStringLiteral("0"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
}

void TestXrayPagesUi::xmuxDisabledIsNotValidated()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    m_model->setXmuxMaxConcurrencyMin(QStringLiteral("1"));
    m_model->setXmuxMaxConnectionsMin(QStringLiteral("1"));
    click(switcherOf(typed("SwitchRowType", "descriptionText", kXmuxRule)));
    QCOMPARE(m_model->xmuxEnabled(), false);
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
}

void TestXrayPagesUi::xmuxTwoErrorsAtOnce()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    m_model->setXmuxMaxConcurrencyMin(QStringLiteral("1"));
    m_model->setXmuxMaxConnectionsMin(QStringLiteral("1"));
    m_model->setSni(QStringLiteral("*"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    click(saveButton());
    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().first().toString(),
             QStringLiteral("SNI: enter a valid IP address or domain name\n") + kXmuxError);
}

void TestXrayPagesUi::xmuxKeepAlive_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("expected");
    QTest::newRow("lone minus") << "-" << "";
    QTest::newRow("negative") << "-30" << "-30";
    QTest::newRow("leading zeros") << "007" << "7";
    QTest::newRow("minus zero") << "-0" << "0";
    QTest::newRow("above int32") << "99999999999" << "2147483647";
    QTest::newRow("below int32") << "-99999999999" << "-2147483648";
    QTest::newRow("inner minus blocked") << "12-3" << "123";
    QTest::newRow("empty") << "" << "";
}

void TestXrayPagesUi::xmuxKeepAlive()
{
    QFETCH(QString, typed);
    QFETCH(QString, expected);

    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(field(QStringLiteral("hKeepAlivePeriod")), typed);
    QCOMPARE(m_model->xmuxHKeepAlivePeriod(), expected);
}

void TestXrayPagesUi::xmuxEditIsDeviceOnly()
{
    XRAY_LOAD("PageProtocolXrayXmuxSettings");
    clearAndType(minMaxInput(minMaxRow(2), QStringLiteral("Min")), QStringLiteral("5"));
    QCOMPARE(m_model->xmuxCMaxReuseTimesMin(), QStringLiteral("5"));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(1).toString(), kDeviceOnlyText);
    answer(questions, false);
}
