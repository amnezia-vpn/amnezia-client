#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"

using namespace amnezia;

namespace
{
    const QString kVision = QStringLiteral("xtls-rprx-vision");
    const QString kUdp443 = QStringLiteral("xtls-rprx-vision-udp443");
    const QString kVisionHint =
            QStringLiteral("xtls-rprx-vision is available only with the RAW (TCP) transport and TLS or Reality security.");
    const QString kDeviceOnlyText =
            QStringLiteral("The server will not be changed now. The new settings apply the next time you connect.");
    const QString kServerBreaksSharedText = QStringLiteral(
            "The server configuration will be updated. All users with whom you shared a connection with will no "
            "longer be able to connect to it. You will need to share the connection again.");
}

void TestXrayPagesUi::flowRadioRoundTrip()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    auto checkedOnly = [this](const QString &text) {
        for (const QString &t : { QStringLiteral("Empty"), kVision, kUdp443 }) {
            if (typed("RadioItemType", "text", t)->property("checked").toBool() != (t == text))
                return false;
        }
        return true;
    };
    QVERIFY(checkedOnly(QStringLiteral("Empty")));
    QVERIFY(!saveButton()->isEnabled());

    click(typed("RadioItemType", "text", kVision));
    QCOMPARE(m_model->flow(), kVision);
    QVERIFY(checkedOnly(kVision));
    QVERIFY(saveButton()->isEnabled());

    click(typed("RadioItemType", "text", kUdp443));
    QCOMPARE(m_model->flow(), kUdp443);
    QVERIFY(checkedOnly(kUdp443));

    click(typed("RadioItemType", "text", QStringLiteral("Empty")));
    QCOMPARE(m_model->flow(), QString());
    QVERIFY(checkedOnly(QStringLiteral("Empty")));
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::flowUdp443Subtitle()
{
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    QCOMPARE(typed("RadioItemType", "text", kUdp443)->property("subtitle").toString(),
             QStringLiteral("For apps that require QUIC. Browsers may stop loading."));
}

void TestXrayPagesUi::flowVisionNotAllowed_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("flow");
    QTest::newRow("xhttp + reality + vision") << "xhttp" << "reality" << kVision;
    QTest::newRow("mkcp + tls + vision") << "mkcp" << "tls" << kVision;
    QTest::newRow("raw + none + udp443") << "raw" << "none" << kUdp443;
    QTest::newRow("mkcp + none + empty") << "mkcp" << "none" << "";
}

void TestXrayPagesUi::flowVisionNotAllowed()
{
    QFETCH(QString, transport);
    QFETCH(QString, security);
    QFETCH(QString, flow);

    openStored(xraytest::baseConfig(transport, security, flow));
    XRAY_LOAD("PageProtocolXrayFlowSettings");

    QVERIFY(typed("RadioItemType", "text", QStringLiteral("Empty"))->property("checked").toBool());
    for (const QString &t : { kVision, kUdp443 }) {
        QQuickItem *radio = typed("RadioItemType", "text", t);
        QVERIFY2(!radio->isEnabled(), qPrintable(t));
        QVERIFY2(!radio->property("checked").toBool(), qPrintable(t));
    }
    QQuickItem *hint = typed("SmallTextType", "text", kVisionHint);
    QVERIFY(hint && hint->isVisible());
    QCOMPARE(m_model->flow(), flow);
    QVERIFY(!m_model->hasUnsavedChanges());

    click(typed("RadioItemType", "text", kVision));
    QCOMPARE(m_model->flow(), flow);
}

void TestXrayPagesUi::flowEmptyWhenVisionNotAllowed()
{
    openStored(xraytest::baseConfig("xhttp", "reality", kVision));
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    click(typed("RadioItemType", "text", QStringLiteral("Empty")));
    QCOMPARE(m_model->flow(), QString());
    QVERIFY(saveButton()->isEnabled());

    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(1).toString(), kDeviceOnlyText);
    answer(questions, false);
}

void TestXrayPagesUi::flowDeviceOrServerChange_data()
{
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QString>("expected");
    QTest::newRow("vision -> udp443 is device-only") << kVision << kUdp443 << kDeviceOnlyText;
    QTest::newRow("empty -> vision breaks shared configs") << "" << kVision << kServerBreaksSharedText;
}

void TestXrayPagesUi::flowDeviceOrServerChange()
{
    QFETCH(QString, from);
    QFETCH(QString, to);
    QFETCH(QString, expected);

    openStored(xraytest::baseConfig("raw", "tls", from));
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    click(typed("RadioItemType", "text", to));
    QCOMPARE(m_model->flow(), to);

    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(1).toString(), expected);
    answer(questions, false);
}

void TestXrayPagesUi::flowEmptyTransportCountsAsRaw()
{
    XrayServerConfig srv = xraytest::baseConfig("raw", "tls", "");
    srv.transport.clear();
    srv.isThirdPartyConfig = true;
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    QCOMPARE(m_model->transport(), QString());
    QVERIFY(typed("RadioItemType", "text", kVision)->isEnabled());
    QVERIFY(typed("RadioItemType", "text", kUdp443)->isEnabled());
}

void TestXrayPagesUi::flowUnknownStoredFlow()
{
    openStored(xraytest::baseConfig("raw", "tls", "xtls-rprx-direct"));
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    for (const QString &t : { QStringLiteral("Empty"), kVision, kUdp443 })
        QVERIFY2(!typed("RadioItemType", "text", t)->property("checked").toBool(), qPrintable(t));
}

void TestXrayPagesUi::flowKeyboard()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXrayFlowSettings");
    QQuickItem *radio = typed("RadioItemType", "text", kVision);
    radio->forceActiveFocus();
    key(Qt::Key_Return);
    QCOMPARE(m_model->flow(), kVision);
}

void TestXrayPagesUi::paddingInitialState()
{
    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QQuickItem *obfs = switcherOf(typed("SwitchRowType", "text", QStringLiteral("xPaddingObfsMode")));
    QVERIFY(obfs && obfs->property("checked").toBool());
    QCOMPARE(typed("DropDownType", "headerText", QStringLiteral("xPaddingPlacement"))->property("text").toString(),
             QStringLiteral("Cookie"));
    QCOMPARE(typed("DropDownType", "headerText", QStringLiteral("xPaddingMethod"))->property("text").toString(),
             QStringLiteral("Repeat-x"));
    QQuickItem *keyInput = field(QStringLiteral("xPaddingKey"));
    QQuickItem *headerInput = field(QStringLiteral("xPaddingHeader"));
    QCOMPARE(keyInput->property("text").toString(), QString());
    QCOMPARE(keyInput->property("placeholderText").toString(), QStringLiteral("x_padding"));
    QCOMPARE(headerInput->property("text").toString(), QString());
    QCOMPARE(headerInput->property("placeholderText").toString(), QStringLiteral("X-Padding"));
}

void TestXrayPagesUi::paddingSwitchTogglesFields()
{
    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QQuickItem *obfs = switcherOf(typed("SwitchRowType", "text", QStringLiteral("xPaddingObfsMode")));
    QVERIFY(obfs);
    auto dependents = [this] {
        return QList<QQuickItem *> { typed("ListItemType", "text", QStringLiteral("xPaddingBytes")),
                                     typed("TextFieldType", "headerText", QStringLiteral("xPaddingKey")),
                                     typed("TextFieldType", "headerText", QStringLiteral("xPaddingHeader")),
                                     typed("DropDownType", "headerText", QStringLiteral("xPaddingPlacement")),
                                     typed("DropDownType", "headerText", QStringLiteral("xPaddingMethod")) };
    };

    click(obfs);
    QCOMPARE(m_model->xPaddingObfsMode(), false);
    for (QQuickItem *dep : dependents())
        QVERIFY(dep && !dep->isEnabled());
    QVERIFY(saveButton()->isEnabled());

    click(obfs);
    QCOMPARE(m_model->xPaddingObfsMode(), true);
    for (QQuickItem *dep : dependents())
        QVERIFY(dep && dep->isEnabled());
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::paddingSwitchLabelToggles()
{
    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QQuickItem *label = typed("BodyTextType", "text", QStringLiteral("xPaddingObfsMode"));
    QVERIFY(label);
    click(label);
    QCOMPARE(m_model->xPaddingObfsMode(), false);
}

void TestXrayPagesUi::paddingBytesSubtitle_data()
{
    QTest::addColumn<QString>("min");
    QTest::addColumn<QString>("max");
    QTest::addColumn<QString>("subtitle");
    QTest::newRow("both empty") << "" << "" << "Default";
    QTest::newRow("min only") << "200" << "" << "200—1000";
    QTest::newRow("max only") << "" << "500" << "100—500";
    QTest::newRow("both") << "200" << "400" << "200—400";
}

void TestXrayPagesUi::paddingBytesSubtitle()
{
    QFETCH(QString, min);
    QFETCH(QString, max);
    QFETCH(QString, subtitle);

    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    m_model->setXPaddingBytesMin(min);
    m_model->setXPaddingBytesMax(max);
    QCOMPARE(typed("ListItemType", "text", QStringLiteral("xPaddingBytes"))->property("subtitle").toString(), subtitle);
}

void TestXrayPagesUi::paddingBytesRowNavigates_data()
{
    QTest::addColumn<bool>("obfs");
    QTest::newRow("obfs on navigates") << true;
    QTest::newRow("obfs off does nothing") << false;
}

void TestXrayPagesUi::paddingBytesRowNavigates()
{
    QFETCH(bool, obfs);

    XrayServerConfig srv = xraytest::baseConfig("xhttp", "reality", "");
    srv.xhttp.xPadding.obfsMode = obfs;
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(typed("ListItemType", "text", QStringLiteral("xPaddingBytes")));

    QCOMPARE(pages.count(), obfs ? 1 : 0);
    if (obfs) {
        QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)),
                 PageLoader::PageEnum::PageProtocolXrayXPaddingBytesSettings);
    }
}

void TestXrayPagesUi::paddingKeyHeaderFilter_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("expected");
    QTest::newRow("key") << "xPaddingKey" << "my.key name$1" << "mykeyname1";
    QTest::newRow("header") << "xPaddingHeader" << "X-Pad.ding" << "X-Padding";
}

void TestXrayPagesUi::paddingKeyHeaderFilter()
{
    QFETCH(QString, header);
    QFETCH(QString, typed);
    QFETCH(QString, expected);

    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QQuickItem *f = field(header);
    clearAndType(f, typed);
    QCOMPARE(f->property("text").toString(), expected);
    QCOMPARE(header == QLatin1String("xPaddingKey") ? m_model->xPaddingKey() : m_model->xPaddingHeader(), expected);
    QVERIFY(saveButton()->isEnabled());
}

void TestXrayPagesUi::paddingDropDownRoundTrip_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("option");
    for (const QString o : { QStringLiteral("Cookie"), QStringLiteral("Header"), QStringLiteral("Query"),
                             QStringLiteral("Query in header") })
        QTest::newRow(qPrintable("placement = " + o)) << "xPaddingPlacement" << o;
    for (const QString o : { QStringLiteral("Repeat-x"), QStringLiteral("Tokenish") })
        QTest::newRow(qPrintable("method = " + o)) << "xPaddingMethod" << o;
}

void TestXrayPagesUi::paddingDropDownRoundTrip()
{
    QFETCH(QString, header);
    QFETCH(QString, option);

    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QQuickItem *dd = typed("DropDownType", "headerText", header);
    QVERIFY(dd);
    const QStringList all = optionNames(dd, false);
    const QStringList expected = header == QLatin1String("xPaddingPlacement")
            ? QStringList { "Cookie", "Header", "Query", "Query in header" }
            : QStringList { "Repeat-x", "Tokenish" };
    QCOMPARE(all, expected);
    QVERIFY(chooseOption(dd, option));
    QCOMPARE(header == QLatin1String("xPaddingPlacement") ? m_model->xPaddingPlacement() : m_model->xPaddingMethod(),
             option);
    QCOMPARE(dd->property("text").toString(), option);
    QCOMPARE(optionNames(dd, true), QStringList { option });
}

void TestXrayPagesUi::paddingDisabledDropDownDoesNotOpen()
{
    XrayServerConfig srv = xraytest::baseConfig("xhttp", "reality", "");
    srv.xhttp.xPadding.obfsMode = false;
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayXPaddingSettings");
    QQuickItem *dd = typed("DropDownType", "headerText", QStringLiteral("xPaddingPlacement"));
    QObject *popup = popupOf(dd);
    QVERIFY(popup);
    click(dd);
    QTest::qWait(400);
    QVERIFY(!popup->property("opened").toBool());
}

void TestXrayPagesUi::paddingBytesInitialState()
{
    XRAY_LOAD("PageProtocolXrayXPaddingBytesSettings");
    QQuickItem *row = minMaxRow(0);
    QQuickItem *minInput = minMaxInput(row, QStringLiteral("Min"));
    QQuickItem *maxInput = minMaxInput(row, QStringLiteral("Max"));
    QVERIFY(minInput && maxInput);
    QCOMPARE(minInput->property("text").toString(), QString());
    QCOMPARE(maxInput->property("text").toString(), QString());
    QCOMPARE(minInput->property("placeholderText").toString(), QStringLiteral("100"));
    QCOMPARE(maxInput->property("placeholderText").toString(), QStringLiteral("1000"));
    QVERIFY(typed("SmallTextType", "text", QStringLiteral("≤ 2147483647"))->isVisible());
    QVERIFY(typed("SmallTextType", "text", QStringLiteral("Range"))->isVisible());
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::paddingBytesRange_data()
{
    QTest::addColumn<QString>("minBefore");
    QTest::addColumn<QString>("maxBefore");
    QTest::addColumn<QString>("which");
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("minAfter");
    QTest::addColumn<QString>("maxAfter");
    QTest::newRow("min only") << "" << "" << "Min" << "200" << "200" << "";
    QTest::newRow("max only") << "" << "" << "Max" << "400" << "" << "400";
    QTest::newRow("min above max placeholder raises max") << "" << "" << "Min" << "2000" << "2000" << "2000";
    QTest::newRow("min above max raises max") << "" << "400" << "Min" << "500" << "500" << "500";
    QTest::newRow("max below min is raised") << "300" << "" << "Max" << "200" << "300" << "300";
    QTest::newRow("max below min placeholder is raised") << "" << "" << "Max" << "50" << "" << "100";
    QTest::newRow("leading zeros") << "" << "" << "Min" << "007" << "7" << "";
    QTest::newRow("clear min") << "200" << "" << "Min" << "" << "" << "";
}

void TestXrayPagesUi::paddingBytesRange()
{
    QFETCH(QString, minBefore);
    QFETCH(QString, maxBefore);
    QFETCH(QString, which);
    QFETCH(QString, typed);
    QFETCH(QString, minAfter);
    QFETCH(QString, maxAfter);

    XRAY_LOAD("PageProtocolXrayXPaddingBytesSettings");
    m_model->setXPaddingBytesMin(minBefore);
    m_model->setXPaddingBytesMax(maxBefore);
    QQuickItem *row = minMaxRow(0);
    clearAndType(minMaxInput(row, which), typed);

    QCOMPARE(m_model->xPaddingBytesMin(), minAfter);
    QCOMPARE(m_model->xPaddingBytesMax(), maxAfter);
    QCOMPARE(minMaxInput(row, QStringLiteral("Max"))->property("text").toString(), maxAfter);
}

void TestXrayPagesUi::paddingBytesOverflowAndFilter()
{
    XRAY_LOAD("PageProtocolXrayXPaddingBytesSettings");
    QQuickItem *minInput = minMaxInput(minMaxRow(0), QStringLiteral("Min"));
    clearAndType(minInput, QStringLiteral("9999999999"), false);
    QCOMPARE(minInput->property("text").toString(), QStringLiteral("999999999"));
    QCOMPARE(m_model->xPaddingBytesMin(), QStringLiteral("999999999"));

    clearAndType(minInput, QStringLiteral("-12a"), false);
    QCOMPARE(minInput->property("text").toString(), QStringLiteral("12"));
}
