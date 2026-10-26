#include "testXrayPagesUi.h"

using namespace amnezia;

namespace
{
    const QString kSni = QStringLiteral("Server Name (SNI)");
    const QString kSniError = QStringLiteral("Enter a valid IP address or domain name");
    const QString kDeviceOnlyText =
            QStringLiteral("The server will not be changed now. The new settings apply the next time you connect.");
    const QString kServerBreaksSharedText = QStringLiteral(
            "The server configuration will be updated. All users with whom you shared a connection with will no "
            "longer be able to connect to it. You will need to share the connection again.");
    const QStringList kFingerprints = { "chrome", "firefox", "safari", "ios", "android", "edge", "360", "qq", "random" };
    const QStringList kAlpn = { "h2", "http/1.1", "h2,http/1.1" };
}

void TestXrayPagesUi::securityMatrix_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("checked");
    QTest::addColumn<bool>("realityEnabled");
    QTest::addColumn<bool>("hint");
    QTest::addColumn<bool>("alpn");
    QTest::addColumn<bool>("tlsFingerprint");
    QTest::addColumn<bool>("realityFingerprint");
    QTest::addColumn<int>("sniFields");
    for (const QString t : { QStringLiteral("raw"), QStringLiteral("xhttp") }) {
        QTest::newRow(qPrintable(t + " + none")) << t << "none" << "None" << true << false << false << false << false << 0;
        QTest::newRow(qPrintable(t + " + tls")) << t << "tls" << "TLS" << true << false << true << true << false << 1;
        QTest::newRow(qPrintable(t + " + reality")) << t << "reality" << "Reality" << true << false << false << false << true << 1;
    }
    QTest::newRow("mkcp + none") << "mkcp" << "none" << "None" << false << true << false << false << false << 0;
    QTest::newRow("mkcp + tls") << "mkcp" << "tls" << "TLS" << false << true << true << false << false << 1;
    QTest::newRow("mkcp + reality shows None") << "mkcp" << "reality" << "None" << false << true << false << false << false << 0;
}

void TestXrayPagesUi::securityMatrix()
{
    QFETCH(QString, transport);
    QFETCH(QString, security);
    QFETCH(QString, checked);
    QFETCH(bool, realityEnabled);
    QFETCH(bool, hint);
    QFETCH(bool, alpn);
    QFETCH(bool, tlsFingerprint);
    QFETCH(bool, realityFingerprint);
    QFETCH(int, sniFields);

    openStored(xraytest::baseConfig(transport, security, ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");

    for (const QString &text : { QStringLiteral("None"), QStringLiteral("TLS"), QStringLiteral("Reality") }) {
        QQuickItem *radio = typed("RadioItemType", "text", text);
        QVERIFY(radio);
        QCOMPARE(radio->property("checked").toBool(), text == checked);
    }
    QCOMPARE(typed("RadioItemType", "text", QStringLiteral("Reality"))->isEnabled(), realityEnabled);
    QCOMPARE(typed("SmallTextType", "text",
                   QStringLiteral("REALITY is not supported with the mKCP transport. Use None or TLS."))->isVisible(),
             hint);
    QCOMPARE(typed("DropDownType", "descriptionText", QStringLiteral("ALPN"))->isVisible(), alpn);

    QList<QQuickItem *> fingerprints;
    for (QQuickItem *dd : allOfType("DropDownType")) {
        if (dd->property("descriptionText").toString() == QLatin1String("Fingerprint"))
            fingerprints << dd;
    }
    QCOMPARE(fingerprints.size(), 2);
    QCOMPARE(fingerprints.at(0)->isVisible(), tlsFingerprint);
    QCOMPARE(fingerprints.at(1)->isVisible(), realityFingerprint);

    int visibleSni = 0;
    for (QQuickItem *f : allOfType("TextFieldType")) {
        if (f->property("headerText").toString() == kSni && f->isVisible())
            ++visibleSni;
    }
    QCOMPARE(visibleSni, sniFields);
}

void TestXrayPagesUi::securityRadioRoundTrip()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");

    click(typed("RadioItemType", "text", QStringLiteral("TLS")));
    QCOMPARE(m_model->security(), QStringLiteral("tls"));
    QVERIFY(typed("RadioItemType", "text", QStringLiteral("TLS"))->property("checked").toBool());
    QVERIFY(!typed("RadioItemType", "text", QStringLiteral("Reality"))->property("checked").toBool());
    QVERIFY(saveButton()->isEnabled());

    click(typed("RadioItemType", "text", QStringLiteral("None")));
    QCOMPARE(m_model->security(), QStringLiteral("none"));

    click(typed("RadioItemType", "text", QStringLiteral("Reality")));
    QCOMPARE(m_model->security(), QStringLiteral("reality"));
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!saveButton()->isEnabled());
}

void TestXrayPagesUi::securityRealityNotClickableOnMkcp()
{
    openStored(xraytest::baseConfig("mkcp", "tls", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *reality = typed("RadioItemType", "text", QStringLiteral("Reality"));
    QVERIFY(reality && !reality->isEnabled());
    click(reality);
    QCOMPARE(m_model->security(), QStringLiteral("tls"));
    QVERIFY(!m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::securityMkcpStoredReality()
{
    openStored(xraytest::baseConfig("mkcp", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QVERIFY(typed("RadioItemType", "text", QStringLiteral("None"))->property("checked").toBool());
    QCOMPARE(m_model->security(), QStringLiteral("reality"));
    QVERIFY(!m_model->hasUnsavedChanges());

    click(typed("RadioItemType", "text", QStringLiteral("None")));
    QCOMPARE(m_model->security(), QStringLiteral("none"));
    QVERIFY(saveButton()->isEnabled());

    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(1).toString(), kDeviceOnlyText);
    answer(questions, false);
}

void TestXrayPagesUi::securityDropDownOptions_data()
{
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("description");
    QTest::addColumn<QStringList>("expected");
    QTest::addColumn<QString>("current");
    QTest::newRow("tls ALPN") << "tls" << "ALPN" << kAlpn << "h2";
    QTest::newRow("tls Fingerprint") << "tls" << "Fingerprint" << kFingerprints << "chrome";
    QTest::newRow("reality Fingerprint") << "reality" << "Fingerprint" << kFingerprints << "chrome";
}

void TestXrayPagesUi::securityDropDownOptions()
{
    QFETCH(QString, security);
    QFETCH(QString, description);
    QFETCH(QStringList, expected);
    QFETCH(QString, current);

    openStored(xraytest::baseConfig("raw", security, ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *dd = dropDown(description);
    QVERIFY(dd && dd->isVisible());
    QCOMPARE(optionNames(dd, false), expected);
    QCOMPARE(optionNames(dd, true), QStringList { current });
}

void TestXrayPagesUi::securityDropDownRoundTrip_data()
{
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("option");
    for (const QString &o : kAlpn)
        QTest::newRow(qPrintable("ALPN = " + o)) << "tls" << "ALPN" << o;
    for (const QString &o : kFingerprints)
        QTest::newRow(qPrintable("tls Fingerprint = " + o)) << "tls" << "Fingerprint" << o;
    for (const QString &o : kFingerprints)
        QTest::newRow(qPrintable("reality Fingerprint = " + o)) << "reality" << "Fingerprint" << o;
}

void TestXrayPagesUi::securityDropDownRoundTrip()
{
    QFETCH(QString, security);
    QFETCH(QString, description);
    QFETCH(QString, option);

    openStored(xraytest::baseConfig("raw", security, ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *dd = dropDown(description);
    QVERIFY(dd);
    QVERIFY(chooseOption(dd, option));

    QCOMPARE(description == QLatin1String("ALPN") ? m_model->alpn() : m_model->fingerprint(), option);
    QCOMPARE(dd->property("text").toString(), option);
    const bool isDefault = option == QLatin1String("h2") || option == QLatin1String("chrome");
    QCOMPARE(saveButton()->isEnabled(), !isDefault);
    QCOMPARE(optionNames(dd, true), QStringList { option });
}

void TestXrayPagesUi::securityFingerprintSharedBetweenBlocks()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QVERIFY(chooseOption(dropDown(QStringLiteral("Fingerprint")), QStringLiteral("safari")));
    click(typed("RadioItemType", "text", QStringLiteral("TLS")));
    QQuickItem *tlsFingerprint = dropDown(QStringLiteral("Fingerprint"));
    QVERIFY(tlsFingerprint && tlsFingerprint->isVisible());
    QCOMPARE(tlsFingerprint->property("text").toString(), QStringLiteral("safari"));
    QCOMPARE(optionNames(tlsFingerprint, true), QStringList { QStringLiteral("safari") });
}

void TestXrayPagesUi::securityValidSni()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(kSni);
    QVERIFY(sni);
    clearAndType(sni, QStringLiteral("vpn.example.org"));
    QCOMPARE(m_model->sni(), QStringLiteral("vpn.example.org"));
    QCOMPARE(m_model->site(), QStringLiteral("vpn.example.org"));
    QCOMPARE(ownerField(sni)->property("errorText").toString(), QString());
    QVERIFY(saveButton()->isEnabled());
}

void TestXrayPagesUi::securitySniSiteCoupling()
{
    XrayServerConfig srv = xraytest::baseConfig("raw", "reality", "");
    srv.sni = QStringLiteral("a.example.com");
    srv.site = QStringLiteral("b.example.com");
    openStored(srv);
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(kSni);
    QVERIFY(sni);

    click(sni);
    key(Qt::Key_Return);
    QCOMPARE(m_model->site(), QStringLiteral("b.example.com"));
    QVERIFY(!m_model->hasUnsavedChanges());

    key(Qt::Key_End);
    key(Qt::Key_Backspace);
    type(QStringLiteral("m"));
    QCOMPARE(m_model->sni(), QStringLiteral("a.example.com"));
    QCOMPARE(m_model->site(), QStringLiteral("a.example.com"));
    QVERIFY(m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::securityInvalidSni_data()
{
    QTest::addColumn<QString>("value");
    QTest::newRow("no tld: localhost") << "localhost";
    QTest::newRow("no tld: example") << "example";
    QTest::newRow("digits only") << "123";
    QTest::newRow("bad ipv4") << "999.1.1.1";
    QTest::newRow("short ipv4") << "1.2.3";
}

void TestXrayPagesUi::securityInvalidSni()
{
    QFETCH(QString, value);

    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(kSni);
    QQuickItem *sniField = ownerField(sni);
    QVERIFY(sniField);

    clearAndType(sni, value);
    QCOMPARE(m_model->sni(), value);
    QCOMPARE(sniField->property("errorText").toString(), kSniError);

    type(QStringLiteral("x"));
    QCOMPARE(sniField->property("errorText").toString(), QString());

    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    click(saveButton());
    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().first().toString(), QStringLiteral("SNI: enter a valid IP address or domain name"));
}

void TestXrayPagesUi::securityIpv4Sni()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    clearAndType(visibleField(kSni), QStringLiteral("1.2.3.4"));
    QCOMPARE(m_model->sni(), QStringLiteral("1.2.3.4"));

    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
}

void TestXrayPagesUi::securitySniFilter()
{
    openStored(xraytest::baseConfig("raw", "tls", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(kSni);
    clearAndType(sni, QStringLiteral("a b:c/d_e*"), false);
    QCOMPARE(sni->property("text").toString(), QStringLiteral("abcd_e"));
    QCOMPARE(m_model->sni(), QStringLiteral("abcd_e"));
}

void TestXrayPagesUi::securityEmptySni()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(kSni);
    clearAndType(sni, QString());
    QCOMPARE(m_model->sni(), QString());
    QCOMPARE(m_model->site(), QString());
    QCOMPARE(sni->property("placeholderText").toString(), QStringLiteral("www.googletagmanager.com"));

    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
}

void TestXrayPagesUi::securitySniErrorOnFocusOut()
{
    openStored(xraytest::baseConfig("raw", "reality", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    QQuickItem *sni = visibleField(kSni);
    QQuickItem *sniField = ownerField(sni);
    QVERIFY(sniField);

    clearAndType(sni, QStringLiteral("localhost"), false);
    QCOMPARE(sniField->property("errorText").toString(), QString());
    sni->setProperty("focus", false);
    QTest::qWait(50);
    QCOMPARE(sniField->property("errorText").toString(), kSniError);
}

void TestXrayPagesUi::securityNoneIgnoresSni()
{
    openStored(xraytest::baseConfig("raw", "none", ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    m_model->setSni(QStringLiteral("*"));
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(errors.count(), 0);
    QCOMPARE(questions.count(), 1);
    answer(questions, false);
}

void TestXrayPagesUi::securitySaveDialogDescription_data()
{
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("edit");
    QTest::addColumn<QString>("expected");
    QTest::newRow("fingerprint is device-only") << "reality" << "fingerprint" << kDeviceOnlyText;
    QTest::newRow("alpn breaks shared configs") << "tls" << "alpn" << kServerBreaksSharedText;
    QTest::newRow("reality to tls breaks shared configs") << "reality" << "security" << kServerBreaksSharedText;
    QTest::newRow("sni breaks shared configs") << "reality" << "sni" << kServerBreaksSharedText;
}

void TestXrayPagesUi::securitySaveDialogDescription()
{
    QFETCH(QString, security);
    QFETCH(QString, edit);
    QFETCH(QString, expected);

    openStored(xraytest::baseConfig("raw", security, ""));
    XRAY_LOAD("PageProtocolXraySecuritySettings");
    if (edit == QLatin1String("fingerprint"))
        QVERIFY(chooseOption(dropDown(QStringLiteral("Fingerprint")), QStringLiteral("firefox")));
    else if (edit == QLatin1String("alpn"))
        QVERIFY(chooseOption(dropDown(QStringLiteral("ALPN")), QStringLiteral("http/1.1")));
    else if (edit == QLatin1String("security"))
        click(typed("RadioItemType", "text", QStringLiteral("TLS")));
    else if (edit == QLatin1String("sni"))
        clearAndType(visibleField(kSni), QStringLiteral("www.example.org"));

    XRAY_QUESTIONS(questions);
    click(saveButton());
    QCOMPARE(questions.count(), 1);
    QCOMPARE(questions.first().at(0).toString(), QStringLiteral("Save settings?"));
    QCOMPARE(questions.first().at(1).toString(), expected);
    answer(questions, false);
}
