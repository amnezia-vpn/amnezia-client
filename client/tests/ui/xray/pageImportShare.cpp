#include "testXrayPagesUi.h"

using namespace amnezia;

void TestXrayPagesUi::importAddDisabledInitially()
{
    step(QStringLiteral("checks: Add preset is disabled on an empty form"));
    XRAY_LOAD("PageProtocolXrayImportPreset");
    QQuickItem *add = button(QStringLiteral("Add preset"));
    QVERIFY(add);
    QVERIFY(!add->isEnabled());
    QCOMPARE(allOfType("TextAreaType").value(0)->property("errorText").toString(), QString());
}

void TestXrayPagesUi::importInvalidJsonShowsError_data()
{
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QString>("fragment");
    QTest::newRow("syntax") << "\"port\": 443," << "\"port\": 443" << "Line 2: missing comma";
    QTest::newRow("protocol") << "\"vless\"" << "\"vmess\"" << "protocol must be";
    QTest::newRow("real client id") << "\"id\": \"@auto\"" << "\"id\": \"11111111-2222-4333-8444-555555555555\""
                                    << "clients id must be";
}

void TestXrayPagesUi::importInvalidJsonShowsError()
{
    step(QStringLiteral("checks: invalid preset JSON shows an error and disables Add preset"));
    QFETCH(QString, from);
    QFETCH(QString, to);
    QFETCH(QString, fragment);

    XRAY_LOAD("PageProtocolXrayImportPreset");
    clearAndType(field(QStringLiteral("Name")), QStringLiteral("Office"));
    QQuickItem *area = allOfType("TextAreaType").value(0);
    QVERIFY(area);
    area->setProperty("textAreaText", editedInbound(from, to));

    const QString error = area->property("errorText").toString();
    QVERIFY2(error.contains(fragment), qPrintable(error));
    QVERIFY(!button(QStringLiteral("Add preset"))->isEnabled());
}

void TestXrayPagesUi::importValidJsonNeedsName()
{
    step(QStringLiteral("checks: valid JSON still needs a name before Add preset is enabled"));
    XRAY_LOAD("PageProtocolXrayImportPreset");
    QQuickItem *area = allOfType("TextAreaType").value(0);
    QVERIFY(area);
    area->setProperty("textAreaText", m_model->inboundText());

    QCOMPARE(area->property("errorText").toString(), QString());
    QQuickItem *add = button(QStringLiteral("Add preset"));
    QVERIFY(!add->isEnabled());
    clearAndType(field(QStringLiteral("Name")), QStringLiteral("Office"));
    QVERIFY(add->isEnabled());
}

void TestXrayPagesUi::importAddsPreset()
{
    step(QStringLiteral("checks: Add preset stores the preset and closes the page without changing the selection"));
    XRAY_LOAD("PageProtocolXrayImportPreset");
    const QString text = editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443"));
    allOfType("TextAreaType").value(0)->setProperty("textAreaText", text);
    clearAndType(field(QStringLiteral("Name")), QStringLiteral("Office"));
    QSignalSpy closed(m_pages, &PageController::closePage);
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);

    click(button(QStringLiteral("Add preset")));
    QCOMPARE(closed.count(), 1);
    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(), QStringLiteral("Preset added"));
    const QStringList ids = customIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(m_presets->presetName(ids.first()), QStringLiteral("Office"));
    QCOMPARE(m_presets->presetJson(ids.first()), text);
    QCOMPARE(m_model->presetId(), QString::fromLatin1(xrayPresets::defaultPresetId));
}

void TestXrayPagesUi::shareCurrentPreset()
{
    step(QStringLiteral("checks: share page shows the current preset read-only"));
    XRAY_LOAD("PageProtocolXraySharePreset");
    QVERIFY(typed("BodyTextType", "text", m_model->presetName()));
    QQuickItem *editor = codeEditor();
    QVERIFY(editor);
    QVERIFY(editor->property("readOnly").toBool());
    QCOMPARE(editor->property("text").toString(), m_model->inboundText());
}

void TestXrayPagesUi::shareCustomPreset()
{
    step(QStringLiteral("checks: share page shows a custom preset by id"));
    const QString text = editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443"));
    const QString id = m_presets->addPreset(QStringLiteral("Office"), text);
    XRAY_LOAD("PageProtocolXraySharePreset");
    m_page->setProperty("presetId", id);

    QVERIFY(typed("BodyTextType", "text", QStringLiteral("Office")));
    QCOMPARE(codeEditor()->property("text").toString(), text);
}

void TestXrayPagesUi::shareCopyJson()
{
    step(QStringLiteral("checks: Copy JSON puts the preset on the clipboard"));
    XRAY_LOAD("PageProtocolXraySharePreset");
    QGuiApplication::clipboard()->clear();
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);

    click(button(QStringLiteral("Copy JSON")));
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), m_model->inboundText());
    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(), QStringLiteral("Copied"));
}
