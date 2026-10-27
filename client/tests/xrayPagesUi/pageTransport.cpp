#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"

using namespace amnezia;

namespace
{
    const QString kDeviceOnlyText =
            QStringLiteral("The server will not be changed now. The new settings apply the next time you connect.");
    const QString kServerKeepsSharedText = QStringLiteral(
            "The server configuration will be updated. If XRay on the server is outdated, the container will be "
            "recreated with the current version; connections that were already shared keep working.");
    const QString kServerBreaksSharedText = QStringLiteral(
            "The server configuration will be updated. All users with whom you shared a connection with will no "
            "longer be able to connect to it. You will need to share the connection again.");

    QString xhttpValue(const XrayConfigModel *m, const QString &header)
    {
        if (header == QLatin1String("Mode"))
            return m->xhttpMode();
        if (header == QLatin1String("UplinkHTTPMethod"))
            return m->xhttpUplinkMethod();
        if (header == QLatin1String("SessionPlacement"))
            return m->xhttpSessionPlacement();
        if (header == QLatin1String("SeqPlacement"))
            return m->xhttpSeqPlacement();
        if (header == QLatin1String("UplinkDataPlacement"))
            return m->xhttpUplinkDataPlacement();
        if (header == QLatin1String("SessionKey"))
            return m->xhttpSessionKey();
        if (header == QLatin1String("SeqKey"))
            return m->xhttpSeqKey();
        if (header == QLatin1String("UplinkDataKey"))
            return m->xhttpUplinkDataKey();
        if (header == QLatin1String("UplinkChunkSize"))
            return m->xhttpUplinkChunkSize();
        if (header == QLatin1String("scMaxBufferedPosts"))
            return m->xhttpScMaxBufferedPosts();
        if (header == QLatin1String("TTI"))
            return m->mkcpTti();
        if (header == QLatin1String("uplinkCapacity"))
            return m->mkcpUplinkCapacity();
        if (header == QLatin1String("downlinkCapacity"))
            return m->mkcpDownlinkCapacity();
        if (header == QLatin1String("mtu"))
            return m->mkcpMtu();
        if (header == QLatin1String("cwndMultiplier"))
            return m->mkcpCwndMultiplier();
        if (header == QLatin1String("maxSendingWindow"))
            return m->mkcpMaxSendingWindow();
        return QStringLiteral("<unknown field %1>").arg(header);
    }

    XrayServerConfig packetUpTls()
    {
        XrayServerConfig srv = xraytest::baseConfig("xhttp", "tls", "");
        srv.xhttp.mode = QStringLiteral("Packet-up");
        return srv;
    }
}

void TestXrayPagesUi::transportXhttpRadioSubtitle()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *radio = typed("RadioItemType", "text", QStringLiteral("XHTTP"));
    QVERIFY(radio);
    QCOMPARE(radio->property("subtitle").toString(), QStringLiteral("Advanced users"));
}

void TestXrayPagesUi::transportRadioCheckedFollowsModel_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("checkedText");
    QTest::newRow("raw") << "raw" << "RAW (TCP)";
    QTest::newRow("xhttp") << "xhttp" << "XHTTP";
    QTest::newRow("mkcp") << "mkcp" << "mKCP";
}

void TestXrayPagesUi::transportRadioCheckedFollowsModel()
{
    QFETCH(QString, transport);
    QFETCH(QString, checkedText);

    openStored(xraytest::baseConfig(transport, "tls", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    for (const QString &text : { QStringLiteral("RAW (TCP)"), QStringLiteral("XHTTP"), QStringLiteral("mKCP") }) {
        QQuickItem *radio = typed("RadioItemType", "text", text);
        QVERIFY(radio);
        QCOMPARE(radio->property("checked").toBool(), text == checkedText);
    }
}

void TestXrayPagesUi::transportRadioReturnKey()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *radio = typed("RadioItemType", "text", QStringLiteral("RAW (TCP)"));
    QVERIFY(radio);
    radio->forceActiveFocus();
    key(Qt::Key_Return);
    QCOMPARE(m_model->transport(), QStringLiteral("raw"));
}

void TestXrayPagesUi::transportSectionVisibility_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<bool>("mkcpVisible");
    QTest::addColumn<bool>("xhttpVisible");
    QTest::newRow("raw") << "raw" << false << false;
    QTest::newRow("xhttp") << "xhttp" << false << true;
    QTest::newRow("mkcp") << "mkcp" << true << false;
}

void TestXrayPagesUi::transportSectionVisibility()
{
    QFETCH(QString, transport);
    QFETCH(bool, mkcpVisible);
    QFETCH(bool, xhttpVisible);

    openStored(xraytest::baseConfig(transport, "tls", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QCOMPARE(typed("TextFieldType", "headerText", QStringLiteral("TTI"))->isVisible(), mkcpVisible);
    QCOMPARE(typed("SmallTextType", "text", QStringLiteral("mKCP Settings"))->isVisible(), mkcpVisible);
    QCOMPARE(typed("DropDownType", "headerText", QStringLiteral("Mode"))->isVisible(), xhttpVisible);
}

void TestXrayPagesUi::transportRawToXhttpFillsDefaults()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    QCOMPARE(m_model->xhttpMode(), QString());
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    click(typed("RadioItemType", "text", QStringLiteral("XHTTP")));

    QCOMPARE(m_model->transport(), QStringLiteral("xhttp"));
    QCOMPARE(m_model->xhttpMode(), QStringLiteral("Stream-one"));
    QCOMPARE(m_model->xhttpUplinkMethod(), QStringLiteral("POST"));
    QCOMPARE(m_model->xhttpSessionPlacement(), QStringLiteral("Path"));
    QCOMPARE(m_model->xhttpSeqPlacement(), QStringLiteral("Path"));
    QCOMPARE(m_model->xhttpUplinkDataPlacement(), QStringLiteral("Body"));
    QCOMPARE(m_model->xhttpHost(), QStringLiteral("www.googletagmanager.com"));
    QCOMPARE(m_model->xhttpScMaxEachPostBytesMin(), QStringLiteral("1000000"));
    QCOMPARE(m_model->xhttpScMaxEachPostBytesMax(), QStringLiteral("1000000"));
    QCOMPARE(m_model->xhttpScMinPostsIntervalMsMin(), QStringLiteral("100"));
    QCOMPARE(m_model->xhttpScMinPostsIntervalMsMax(), QStringLiteral("800"));
    QCOMPARE(m_model->xhttpScStreamUpServerSecsMin(), QStringLiteral("1"));
    QCOMPARE(m_model->xhttpScStreamUpServerSecsMax(), QStringLiteral("100"));
    QCOMPARE(m_model->xhttpPath(), QString());
    QCOMPARE(typed("DropDownType", "headerText", QStringLiteral("Mode"))->property("text").toString(),
             QStringLiteral("Stream-one"));
}

void TestXrayPagesUi::transportSwitchKeepsXhttpValues()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    m_model->setXhttpHost(QStringLiteral("a.com"));
    click(typed("RadioItemType", "text", QStringLiteral("mKCP")));
    QCOMPARE(m_model->transport(), QStringLiteral("mkcp"));
    QCOMPARE(m_model->xhttpHost(), QStringLiteral("a.com"));
}

void TestXrayPagesUi::transportPacketUpFieldsMoreRows_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("security");
    QTest::addColumn<bool>("visible");
    QTest::newRow("packet-up + reality") << "Packet-up" << "reality" << true;
    QTest::newRow("auto + tls") << "Auto" << "tls" << true;
    QTest::newRow("empty + none") << "" << "none" << true;
    QTest::newRow("empty + reality") << "" << "reality" << false;
    QTest::newRow("stream-up + none") << "Stream-up" << "none" << false;
    QTest::newRow("stream-one + tls") << "Stream-one" << "tls" << false;
}

void TestXrayPagesUi::transportPacketUpFieldsMoreRows()
{
    QFETCH(QString, mode);
    QFETCH(QString, security);
    QFETCH(bool, visible);

    openStored(xraytest::baseConfig("xhttp", security, ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    m_model->setXhttpMode(mode);
    for (const QString &h : { QStringLiteral("SeqPlacement"), QStringLiteral("UplinkDataPlacement") })
        QCOMPARE(typed("DropDownType", "headerText", h)->isVisible(), visible);
    for (const QString &h : { QStringLiteral("SeqKey"), QStringLiteral("UplinkDataKey") })
        QCOMPARE(typed("TextFieldType", "headerText", h)->isVisible(), visible);
}

void TestXrayPagesUi::transportPacketUpFieldsFollowModeLive()
{
    openStored(xraytest::baseConfig("xhttp", "tls", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *seqKey = typed("TextFieldType", "headerText", QStringLiteral("SeqKey"));
    QQuickItem *mode = typed("DropDownType", "headerText", QStringLiteral("Mode"));
    QVERIFY(seqKey && mode);
    QVERIFY(!seqKey->isVisible());

    QVERIFY(chooseOption(mode, QStringLiteral("Packet-up")));
    QVERIFY(seqKey->isVisible());
    QVERIFY(chooseOption(mode, QStringLiteral("Stream-up")));
    QVERIFY(!seqKey->isVisible());
}

void TestXrayPagesUi::transportAlwaysVisibleXhttpControls_data()
{
    QTest::addColumn<QString>("mode");
    QTest::newRow("Auto") << "Auto";
    QTest::newRow("Packet-up") << "Packet-up";
    QTest::newRow("Stream-up") << "Stream-up";
    QTest::newRow("Stream-one") << "Stream-one";
}

void TestXrayPagesUi::transportAlwaysVisibleXhttpControls()
{
    QFETCH(QString, mode);

    XrayServerConfig srv = xraytest::baseConfig("xhttp", "tls", "");
    srv.xhttp.mode = mode;
    openStored(srv);
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    for (const QString &h : { QStringLiteral("SessionPlacement"), QStringLiteral("UplinkHTTPMethod") })
        QVERIFY2(typed("DropDownType", "headerText", h)->isVisible(), qPrintable(h));
    for (const QString &h : { QStringLiteral("SessionKey"), QStringLiteral("Host"), QStringLiteral("Path"),
                              QStringLiteral("UplinkChunkSize"), QStringLiteral("scMaxBufferedPosts") })
        QVERIFY2(typed("TextFieldType", "headerText", h)->isVisible(), qPrintable(h));
    for (const QString &d : { QStringLiteral("noGRPCHeader"), QStringLiteral("noSSEHeader") })
        QVERIFY2(typed("SwitchRowType", "descriptionText", d)->isVisible(), qPrintable(d));
}

void TestXrayPagesUi::transportDropDownOptions_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QStringList>("expected");
    QTest::newRow("Mode") << "Mode" << QStringList { "Auto", "Packet-up", "Stream-up", "Stream-one" };
    QTest::newRow("UplinkHTTPMethod") << "UplinkHTTPMethod" << QStringList { "POST", "PUT", "PATCH" };
    QTest::newRow("SessionPlacement") << "SessionPlacement" << QStringList { "Path", "Header", "Cookie", "Query", "None" };
    QTest::newRow("SeqPlacement") << "SeqPlacement" << QStringList { "Path", "Header", "Cookie", "Query", "None" };
    QTest::newRow("UplinkDataPlacement") << "UplinkDataPlacement" << QStringList { "Body", "Auto", "Header", "Cookie" };
}

void TestXrayPagesUi::transportDropDownOptions()
{
    QFETCH(QString, header);
    QFETCH(QStringList, expected);

    openStored(packetUpTls());
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *dd = typed("DropDownType", "headerText", header);
    QVERIFY(dd);
    QCOMPARE(optionNames(dd, false), expected);
    QCOMPARE(optionNames(dd, true), QStringList { xhttpValue(m_model, header) });
}

void TestXrayPagesUi::transportDropDownRoundTrip_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("option");
    const QList<QPair<QString, QStringList>> lists = {
        { "Mode", { "Auto", "Packet-up", "Stream-up", "Stream-one" } },
        { "UplinkHTTPMethod", { "POST", "PUT", "PATCH" } },
        { "SessionPlacement", { "Path", "Header", "Cookie", "Query", "None" } },
        { "SeqPlacement", { "Path", "Header", "Cookie", "Query", "None" } },
        { "UplinkDataPlacement", { "Body", "Auto", "Header", "Cookie" } },
    };
    for (const auto &list : lists) {
        for (const QString &option : list.second)
            QTest::newRow(qPrintable(list.first + QStringLiteral(" = ") + option)) << list.first << option;
    }
}

void TestXrayPagesUi::transportDropDownRoundTrip()
{
    QFETCH(QString, header);
    QFETCH(QString, option);

    openStored(packetUpTls());
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *dd = typed("DropDownType", "headerText", header);
    QVERIFY(dd);
    QVERIFY(chooseOption(dd, option));

    QCOMPARE(xhttpValue(m_model, header), option);
    QCOMPARE(dd->property("text").toString(), option);
    QCOMPARE(optionNames(dd, true), QStringList { option });
}

void TestXrayPagesUi::transportDropDownEscape()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *dd = typed("DropDownType", "headerText", QStringLiteral("Mode"));
    QObject *popup = openDropDown(dd);
    QVERIFY(popup);
    QVERIFY(closeWithEscape(popup));
    QCOMPARE(m_model->xhttpMode(), QStringLiteral("Stream-one"));
    QVERIFY(!m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::transportChoosingCurrentModeIsNoChange()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QVERIFY(chooseOption(typed("DropDownType", "headerText", QStringLiteral("Mode")), QStringLiteral("Stream-one")));
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::transportPacketUpRealityAllowed()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QVERIFY(chooseOption(typed("DropDownType", "headerText", QStringLiteral("Mode")), QStringLiteral("Packet-up")));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
}

void TestXrayPagesUi::transportSwitches_data()
{
    QTest::addColumn<QString>("description");
    QTest::newRow("noGRPCHeader") << "noGRPCHeader";
    QTest::newRow("noSSEHeader") << "noSSEHeader";
}

void TestXrayPagesUi::transportSwitches()
{
    QFETCH(QString, description);

    XRAY_LOAD("PageProtocolXrayTransportSettings");
    auto value = [this, &description] {
        return description == QLatin1String("noGRPCHeader") ? m_model->xhttpDisableGrpc() : m_model->xhttpDisableSse();
    };
    QQuickItem *sw = switcherOf(typed("SwitchRowType", "descriptionText", description));
    QVERIFY(sw);
    QCOMPARE(value(), true);
    QCOMPARE(sw->property("checked").toBool(), true);

    click(sw);
    QCOMPARE(value(), false);
    QVERIFY(m_model->hasUnsavedChanges());
    QVERIFY(saveButton()->isEnabled());

    click(sw);
    QCOMPARE(value(), true);
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::transportSwitchLabelToggles()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *label = typed("BodyTextType", "text", QStringLiteral("Disable gRPC Header"));
    QVERIFY(label);
    click(label);
    QCOMPARE(m_model->xhttpDisableGrpc(), false);
}

void TestXrayPagesUi::transportHostFilter()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *host = field(QStringLiteral("Host"));
    QVERIFY(host);
    clearAndType(host, QStringLiteral("a b/c*@!"), false);
    QCOMPARE(host->property("text").toString(), QStringLiteral("abc"));
    QCOMPARE(m_model->xhttpHost(), QStringLiteral("abc"));
}

void TestXrayPagesUi::transportHostValidity_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("valid");
    QTest::newRow("domain") << "example.com" << true;
    QTest::newRow("ipv4") << "1.2.3.4" << true;
    QTest::newRow("ipv6") << "::1" << true;
    QTest::newRow("digits only") << "12345" << false;
    QTest::newRow("no tld") << "localhost" << false;
    QTest::newRow("short ipv4") << "1.2.3" << false;
    QTest::newRow("two hosts") << "a.com,b.com" << false;
    QTest::newRow("host with port") << "example.com:443" << false;
}

void TestXrayPagesUi::transportHostValidity()
{
    QFETCH(QString, value);
    QFETCH(bool, valid);

    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *hostField = typed("TextFieldType", "headerText", QStringLiteral("Host"));
    clearAndType(input(hostField), value);

    QCOMPARE(m_model->xhttpHost(), value);
    QCOMPARE(hostField->property("errorText").toString(),
             valid ? QString() : QStringLiteral("Enter a valid IP address or domain name"));
    QCOMPARE(XrayConfigModel::isValidHost(value), valid);

    if (!valid) {
        type(QStringLiteral("x"));
        QCOMPARE(hostField->property("errorText").toString(), QString());
    }
}

void TestXrayPagesUi::transportEmptyHost()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *hostField = typed("TextFieldType", "headerText", QStringLiteral("Host"));
    clearAndType(input(hostField), QString());
    QCOMPARE(m_model->xhttpHost(), QString());
    QCOMPARE(hostField->property("errorText").toString(), QString());
    QCOMPARE(hostField->property("placeholderText").toString(), QStringLiteral("www.googletagmanager.com"));
}

void TestXrayPagesUi::transportSaveRefusesInvalidHost()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    m_model->setXhttpHost(QStringLiteral("12345"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    click(saveButton());
    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().first().toString(), QStringLiteral("Host: enter a valid IP address or domain name"));
}

void TestXrayPagesUi::transportPathFilter()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *path = field(QStringLiteral("Path"));
    clearAndType(path, QStringLiteral("a b|c"), false);
    QCOMPARE(path->property("text").toString(), QStringLiteral("abc"));
    key(Qt::Key_Return);
    QCOMPARE(m_model->xhttpPath(), QStringLiteral("/abc"));
}

void TestXrayPagesUi::transportPathEdgeCases_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("expected");
    QTest::newRow("empty stays empty") << "" << "";
    QTest::newRow("slash kept") << "/x" << "/x";
    QTest::newRow("nested gets slash") << "x/y" << "/x/y";
    QTest::newRow("query kept") << "p?a=1" << "/p?a=1";
}

void TestXrayPagesUi::transportPathEdgeCases()
{
    QFETCH(QString, typed);
    QFETCH(QString, expected);

    XRAY_LOAD("PageProtocolXrayTransportSettings");
    clearAndType(field(QStringLiteral("Path")), typed);
    QCOMPARE(m_model->xhttpPath(), expected);
}

void TestXrayPagesUi::transportKeyFieldsFilter_data()
{
    QTest::addColumn<QString>("header");
    QTest::newRow("SessionKey") << "SessionKey";
    QTest::newRow("SeqKey") << "SeqKey";
    QTest::newRow("UplinkDataKey") << "UplinkDataKey";
}

void TestXrayPagesUi::transportKeyFieldsFilter()
{
    QFETCH(QString, header);

    openStored(packetUpTls());
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *f = field(header);
    QVERIFY(f);
    clearAndType(f, QStringLiteral("a.b c/d=e_-"));
    QCOMPARE(f->property("text").toString(), QStringLiteral("abcde_-"));
    QCOMPARE(xhttpValue(m_model, header), QStringLiteral("abcde_-"));
}

void TestXrayPagesUi::transportNumberFields_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("expected");
    for (const QString h : { QStringLiteral("UplinkChunkSize"), QStringLiteral("scMaxBufferedPosts") }) {
        QTest::newRow(qPrintable(h + QStringLiteral(": above int32 is clamped"))) << h << "9999999999" << "2147483647";
        QTest::newRow(qPrintable(h + QStringLiteral(": leading zeros"))) << h << "007" << "7";
        QTest::newRow(qPrintable(h + QStringLiteral(": letters blocked"))) << h << "12a-3" << "123";
        QTest::newRow(qPrintable(h + QStringLiteral(": zero"))) << h << "0" << "0";
        QTest::newRow(qPrintable(h + QStringLiteral(": empty"))) << h << "" << "";
    }
}

void TestXrayPagesUi::transportNumberFields()
{
    QFETCH(QString, header);
    QFETCH(QString, typed);
    QFETCH(QString, expected);

    XRAY_LOAD("PageProtocolXrayTransportSettings");
    clearAndType(field(header), typed);
    QCOMPARE(xhttpValue(m_model, header), expected);
}

void TestXrayPagesUi::transportMinMaxRows_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<QString>("minBefore");
    QTest::addColumn<QString>("maxBefore");
    QTest::addColumn<QString>("which");
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("minAfter");
    QTest::addColumn<QString>("maxAfter");
    QTest::newRow("minPosts: min above max raises max") << 2 << "100" << "800" << "Min" << "900" << "900" << "900";
    QTest::newRow("minPosts: max below min is raised") << 2 << "100" << "800" << "Max" << "50" << "100" << "100";
    QTest::newRow("minPosts: empty min uses placeholder") << 2 << "" << "800" << "Max" << "50" << "" << "100";
    QTest::newRow("minPosts: clear min") << 2 << "100" << "800" << "Min" << "" << "" << "800";
    QTest::newRow("minPosts: leading zeros") << 2 << "100" << "800" << "Min" << "007" << "7" << "800";
    QTest::newRow("streamUp: min above empty max placeholder") << 1 << "1" << "" << "Min" << "200" << "200" << "200";
    QTest::newRow("eachPost: max below min") << 0 << "1000000" << "1000000" << "Max" << "5" << "1000000" << "1000000";
}

void TestXrayPagesUi::transportMinMaxRows()
{
    QFETCH(int, row);
    QFETCH(QString, minBefore);
    QFETCH(QString, maxBefore);
    QFETCH(QString, which);
    QFETCH(QString, typed);
    QFETCH(QString, minAfter);
    QFETCH(QString, maxAfter);

    XRAY_LOAD("PageProtocolXrayTransportSettings");
    auto setRow = [this, row](const QString &mn, const QString &mx) {
        if (row == 0) {
            m_model->setXhttpScMaxEachPostBytesMin(mn);
            m_model->setXhttpScMaxEachPostBytesMax(mx);
        } else if (row == 1) {
            m_model->setXhttpScStreamUpServerSecsMin(mn);
            m_model->setXhttpScStreamUpServerSecsMax(mx);
        } else {
            m_model->setXhttpScMinPostsIntervalMsMin(mn);
            m_model->setXhttpScMinPostsIntervalMsMax(mx);
        }
    };
    auto rowValues = [this, row] {
        if (row == 0)
            return qMakePair(m_model->xhttpScMaxEachPostBytesMin(), m_model->xhttpScMaxEachPostBytesMax());
        if (row == 1)
            return qMakePair(m_model->xhttpScStreamUpServerSecsMin(), m_model->xhttpScStreamUpServerSecsMax());
        return qMakePair(m_model->xhttpScMinPostsIntervalMsMin(), m_model->xhttpScMinPostsIntervalMsMax());
    };
    setRow(minBefore, maxBefore);

    QQuickItem *rowItem = minMaxRow(row);
    QVERIFY(rowItem);
    clearAndType(minMaxInput(rowItem, which), typed);

    QCOMPARE(rowValues().first, minAfter);
    QCOMPARE(rowValues().second, maxAfter);
}

void TestXrayPagesUi::transportMinMaxOverflow()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *minInput = minMaxInput(minMaxRow(2), QStringLiteral("Min"));
    QVERIFY(minInput);
    clearAndType(minInput, QStringLiteral("9999999999"), false);
    QCOMPARE(minInput->property("text").toString(), QStringLiteral("999999999"));
    QCOMPARE(m_model->xhttpScMinPostsIntervalMsMin(), QStringLiteral("999999999"));

    clearAndType(minInput, QStringLiteral("-12a"), false);
    QCOMPARE(minInput->property("text").toString(), QStringLiteral("12"));

    QVERIFY(typed("SmallTextType", "text", QStringLiteral("≤ 2147483647")));
}

void TestXrayPagesUi::transportMkcpPlaceholders_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("placeholder");
    QTest::addColumn<QString>("helper");
    QTest::newRow("TTI") << "TTI" << "50" << "Transmission time interval (ms). Valid range: 10–100.";
    QTest::newRow("uplinkCapacity") << "uplinkCapacity" << "5" << "Uplink capacity (MB/s). Maximum: 2147483647.";
    QTest::newRow("downlinkCapacity") << "downlinkCapacity" << "20" << "Downlink capacity (MB/s). Maximum: 2147483647.";
    QTest::newRow("mtu") << "mtu" << "1350" << "Maximum packet size in bytes. Range: 576–1460.";
    QTest::newRow("cwndMultiplier") << "cwndMultiplier" << "1" << "Congestion window multiplier. Minimum: 1.";
    QTest::newRow("maxSendingWindow") << "maxSendingWindow" << ""
                                      << "Send window in bytes. Must not be smaller than mtu. Leave empty to let Xray decide.";
}

void TestXrayPagesUi::transportMkcpPlaceholders()
{
    QFETCH(QString, header);
    QFETCH(QString, placeholder);
    QFETCH(QString, helper);

    openStored(xraytest::baseConfig("mkcp", "none", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QQuickItem *f = typed("TextFieldType", "headerText", header);
    QVERIFY(f && f->isVisible());
    QCOMPARE(f->property("placeholderText").toString(), placeholder);
    QCOMPARE(f->property("helperText").toString(), helper);
}

void TestXrayPagesUi::transportMkcpClamps_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("mtuBefore");
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("expected");
    QTest::newRow("TTI 101 cannot be typed") << "TTI" << "" << "101" << "10";
    QTest::newRow("TTI 100") << "TTI" << "" << "100" << "100";
    QTest::newRow("TTI 5 -> 10") << "TTI" << "" << "5" << "10";
    QTest::newRow("TTI 0 -> 10") << "TTI" << "" << "0" << "10";
    QTest::newRow("TTI 05 -> 10") << "TTI" << "" << "05" << "10";
    QTest::newRow("TTI empty") << "TTI" << "" << "" << "";
    QTest::newRow("uplink above int32") << "uplinkCapacity" << "" << "9999999999" << "2147483647";
    QTest::newRow("uplink 0") << "uplinkCapacity" << "" << "0" << "0";
    QTest::newRow("downlink above int32") << "downlinkCapacity" << "" << "9999999999" << "2147483647";
    QTest::newRow("mtu 100 -> 576") << "mtu" << "" << "100" << "576";
    QTest::newRow("mtu 9999 -> 1460") << "mtu" << "" << "9999" << "1460";
    QTest::newRow("cwnd 0 -> 1") << "cwndMultiplier" << "" << "0" << "1";
    QTest::newRow("window below mtu 1000") << "maxSendingWindow" << "1000" << "800" << "1000";
    QTest::newRow("window below default mtu") << "maxSendingWindow" << "" << "800" << "1350";
    QTest::newRow("window empty") << "maxSendingWindow" << "" << "" << "";
}

void TestXrayPagesUi::transportMkcpClamps()
{
    QFETCH(QString, header);
    QFETCH(QString, mtuBefore);
    QFETCH(QString, typed);
    QFETCH(QString, expected);

    openStored(xraytest::baseConfig("mkcp", "none", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    m_model->setMkcpMtu(mtuBefore);
    clearAndType(field(header), typed);
    QCOMPARE(xhttpValue(m_model, header), expected);
}

void TestXrayPagesUi::transportMtuRaisesSendingWindow_data()
{
    QTest::addColumn<QString>("windowBefore");
    QTest::addColumn<QString>("mtuTyped");
    QTest::addColumn<QString>("windowAfter");
    QTest::newRow("600 raised to mtu 1000") << "600" << "1000" << "1000";
    QTest::newRow("2000 above mtu kept") << "2000" << "1460" << "2000";
    QTest::newRow("cleared mtu uses 1350") << "1000" << "" << "1350";
    QTest::newRow("empty window stays empty") << "" << "1000" << "";
}

void TestXrayPagesUi::transportMtuRaisesSendingWindow()
{
    QFETCH(QString, windowBefore);
    QFETCH(QString, mtuTyped);
    QFETCH(QString, windowAfter);

    openStored(xraytest::baseConfig("mkcp", "none", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    m_model->setMkcpMaxSendingWindow(windowBefore);
    clearAndType(field(QStringLiteral("mtu")), mtuTyped);
    QCOMPARE(m_model->mkcpMaxSendingWindow(), windowAfter);
}

void TestXrayPagesUi::transportSaveDialogDescription_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("edit");
    QTest::addColumn<QString>("expected");
    QTest::newRow("uplink method is device-only") << "xhttp" << "uplinkMethod" << kDeviceOnlyText;
    QTest::newRow("chunk size is device-only") << "xhttp" << "chunk" << kDeviceOnlyText;
    QTest::newRow("min posts interval is device-only") << "xhttp" << "minPosts" << kDeviceOnlyText;
    QTest::newRow("buffered posts keep shared configs") << "xhttp" << "bufferedPosts" << kServerKeepsSharedText;
    QTest::newRow("stream-up secs keep shared configs") << "xhttp" << "streamUp" << kServerKeepsSharedText;
    QTest::newRow("mkcp tti keeps shared configs") << "mkcp" << "tti" << kServerKeepsSharedText;
    QTest::newRow("path breaks shared configs") << "xhttp" << "path" << kServerBreaksSharedText;
    QTest::newRow("host breaks shared configs") << "xhttp" << "host" << kServerBreaksSharedText;
    QTest::newRow("noGRPC breaks shared configs") << "xhttp" << "grpc" << kServerBreaksSharedText;
    QTest::newRow("transport breaks shared configs") << "xhttp" << "transport" << kServerBreaksSharedText;
}

void TestXrayPagesUi::transportSaveDialogDescription()
{
    QFETCH(QString, transport);
    QFETCH(QString, edit);
    QFETCH(QString, expected);

    openStored(xraytest::baseConfig(transport, "tls", ""));
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    if (edit == QLatin1String("uplinkMethod"))
        m_model->setXhttpUplinkMethod(QStringLiteral("PUT"));
    else if (edit == QLatin1String("chunk"))
        m_model->setXhttpUplinkChunkSize(QStringLiteral("4096"));
    else if (edit == QLatin1String("minPosts"))
        m_model->setXhttpScMinPostsIntervalMsMin(QStringLiteral("5"));
    else if (edit == QLatin1String("bufferedPosts"))
        m_model->setXhttpScMaxBufferedPosts(QStringLiteral("5"));
    else if (edit == QLatin1String("streamUp"))
        m_model->setXhttpScStreamUpServerSecsMax(QStringLiteral("50"));
    else if (edit == QLatin1String("tti"))
        m_model->setMkcpTti(QStringLiteral("20"));
    else if (edit == QLatin1String("path"))
        m_model->setXhttpPath(QStringLiteral("/other"));
    else if (edit == QLatin1String("host"))
        m_model->setXhttpHost(QStringLiteral("cdn.example.com"));
    else if (edit == QLatin1String("grpc"))
        m_model->setXhttpDisableGrpc(false);
    else if (edit == QLatin1String("transport"))
        click(typed("RadioItemType", "text", QStringLiteral("RAW (TCP)")));

    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(0).toString(), QStringLiteral("Save settings?"));
    QCOMPARE(questions.first().at(1).toString(), expected);
    answer(questions, false);
}

void TestXrayPagesUi::transportSubpageRows()
{
    XRAY_LOAD("PageProtocolXrayTransportSettings");
    QSignalSpy pages(m_pages, &PageController::goToPage);

    click(typed("ListItemType", "text", QStringLiteral("xPadding")));
    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.at(0).at(0)), PageLoader::PageEnum::PageProtocolXrayXPaddingSettings);

    QQuickItem *xmuxRow = typed("ListItemType", "text", QStringLiteral("XMux"));
    QCOMPARE(xmuxRow->property("subtitle").toString(), QStringLiteral("On"));
    m_model->setXmuxEnabled(false);
    QCOMPARE(xmuxRow->property("subtitle").toString(), QStringLiteral("Off"));
    click(xmuxRow);
    QCOMPARE(pages.count(), 2);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.at(1).at(0)), PageLoader::PageEnum::PageProtocolXrayXmuxSettings);
}

void TestXrayPagesUi::transportReadOnly()
{
    selectServer(m_readOnlyServerId);
    storeAndOpen(xraytest::baseConfig("xhttp", "reality", ""), m_readOnlyServerId);
    XRAY_LOAD("PageProtocolXrayTransportSettings");

    QVERIFY(!saveButton()->isVisible());
    for (const QString &text : { QStringLiteral("RAW (TCP)"), QStringLiteral("XHTTP"), QStringLiteral("mKCP") })
        QVERIFY2(!typed("RadioItemType", "text", text)->isEnabled(), qPrintable(text));
    click(typed("RadioItemType", "text", QStringLiteral("RAW (TCP)")));
    QCOMPARE(m_model->transport(), QStringLiteral("xhttp"));
}
