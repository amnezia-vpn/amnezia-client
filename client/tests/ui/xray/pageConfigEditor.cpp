#include "testXrayPagesUi.h"

using namespace amnezia;

void TestXrayPagesUi::editorShowsDraft()
{
    step(QStringLiteral("checks: editor shows the draft, title, valid status and enabled Done"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");

    QQuickItem *editor = codeEditor();
    QVERIFY(editor);
    QCOMPARE(editor->property("text").toString(), m_model->inboundText());
    QCOMPARE(editor->property("errorLine").toInt(), 0);
    QVERIFY(typed("NavigationBarType", "title", m_model->presetName()));
    QVERIFY(typed("SmallTextType", "text", QStringLiteral("JSON is valid. XRay core rules are met")));
    QVERIFY(button(QStringLiteral("Done"))->isEnabled());
}

void TestXrayPagesUi::editorTypingBreaksJson()
{
    step(QStringLiteral("checks: typing after the JSON shows a syntax error line and disables Done"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    QQuickItem *editor = codeEditor();
    QQuickItem *area = codeArea(editor);
    QVERIFY(area);

    area->forceActiveFocus();
    QTRY_VERIFY(area->hasActiveFocus());
    area->setProperty("cursorPosition", area->property("length"));
    type(QStringLiteral("x"));

    QVERIFY(m_model->draftModified());
    QVERIFY(!m_model->draftValid());
    QVERIFY(m_model->draftErrorLine() > 0);
    QCOMPARE(editor->property("errorLine").toInt(), m_model->draftErrorLine());
    QVERIFY2(m_model->draftStatus().startsWith(QStringLiteral("Line %1:").arg(m_model->draftErrorLine())),
             qPrintable(m_model->draftStatus()));
    QVERIFY(typed("SmallTextType", "text", m_model->draftStatus()));
    QVERIFY(!button(QStringLiteral("Done"))->isEnabled());
}

void TestXrayPagesUi::editorRuleErrors_data()
{
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QString>("fragment");
    QTest::newRow("protocol") << "\"vless\"" << "\"vmess\"" << "protocol must be";
    QTest::newRow("port") << "\"port\": 443" << "\"port\": 70000" << "port must be";
    QTest::newRow("client id") << "\"id\": \"@auto\"" << "\"id\": \"@auto1\"" << "clients id must be";
    QTest::newRow("flow") << "xtls-rprx-vision" << "xtls-rprx-direct" << "unknown flow";
    QTest::newRow("decryption") << "\"decryption\": \"none\"" << "\"decryption\": \"none1\"" << "decryption must be";
    QTest::newRow("security none") << "\"security\": \"reality\"" << "\"security\": \"none\"" << "security none";
}

void TestXrayPagesUi::editorRuleErrors()
{
    step(QStringLiteral("checks: XRay rule violation is reported on its line and disables Done"));
    QFETCH(QString, from);
    QFETCH(QString, to);
    QFETCH(QString, fragment);

    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    const QString text = editedInbound(from, to);
    QVERIFY(text != m_model->inboundText());
    m_model->setDraft(text);

    QQuickItem *editor = codeEditor();
    QCOMPARE(editor->property("text").toString(), text);
    QVERIFY(!m_model->draftValid());
    QVERIFY2(m_model->draftStatus().contains(fragment), qPrintable(m_model->draftStatus()));
    QCOMPARE(editor->property("errorLine").toInt(), m_model->draftErrorLine());
    QVERIFY(!button(QStringLiteral("Done"))->isEnabled());
}

void TestXrayPagesUi::editorFormat()
{
    step(QStringLiteral("checks: Format re-indents compact JSON"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    m_model->setDraft(xrayInbound::normalized(m_model->inboundText()));
    QVERIFY(m_model->draftModified());

    click(button(QStringLiteral("Format")));
    QCOMPARE(m_model->draftText(), m_model->inboundText());
    QCOMPARE(codeEditor()->property("text").toString(), m_model->inboundText());
    QVERIFY(!m_model->draftModified());
}

void TestXrayPagesUi::editorFormatRefusesBrokenJson()
{
    step(QStringLiteral("checks: Format on broken JSON shows a hint and keeps the text"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    const QString broken = m_model->inboundText() + QStringLiteral("x");
    m_model->setDraft(broken);
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);

    click(button(QStringLiteral("Format")));
    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(), QStringLiteral("Fix the JSON syntax first"));
    QCOMPARE(m_model->draftText(), broken);
}

void TestXrayPagesUi::editorDoneCommits()
{
    step(QStringLiteral("checks: Done applies the draft and closes the editor"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    m_model->setDraft(editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    QSignalSpy closed(m_pages, &PageController::closePage);

    click(button(QStringLiteral("Done")));
    QCOMPARE(closed.count(), 1);
    QCOMPARE(m_model->port(), 8443);
    QVERIFY(m_model->presetEdited());
    QVERIFY(m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::editorBackWithoutChanges()
{
    step(QStringLiteral("checks: back without edits closes the editor without a dialog"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    QSignalSpy closed(m_pages, &PageController::closePage);

    click(iconButton(QStringLiteral("arrow-left.svg")));
    QCOMPARE(closed.count(), 1);
    QVERIFY(!alert(QStringLiteral("Discard changes?"))->property("visible").toBool());
}

void TestXrayPagesUi::editorBackDiscardCancelled()
{
    step(QStringLiteral("checks: back with edits asks to discard, Cancel stays in the editor"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    m_model->setDraft(editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    QSignalSpy closed(m_pages, &PageController::closePage);
    click(iconButton(QStringLiteral("arrow-left.svg")));

    QObject *dialog = alert(QStringLiteral("Discard changes?"));
    QVERIFY(waitOpened(dialog));
    QCOMPARE(dialog->property("descriptionText").toString(), QStringLiteral("Your edits to the config will be lost"));
    QCOMPARE(dialog->property("confirmButtonText").toString(), QStringLiteral("Discard"));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Cancel")));

    QCOMPARE(closed.count(), 0);
    QVERIFY(m_model->draftModified());
}

void TestXrayPagesUi::editorBackDiscardConfirmed()
{
    step(QStringLiteral("checks: Discard closes the editor and keeps the saved config"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    m_model->setDraft(editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    QSignalSpy closed(m_pages, &PageController::closePage);
    click(iconButton(QStringLiteral("arrow-left.svg")));

    QObject *dialog = alert(QStringLiteral("Discard changes?"));
    QVERIFY(waitOpened(dialog));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Discard")));

    QCOMPARE(closed.count(), 1);
    QCOMPARE(m_model->port(), 443);
    QVERIFY(!m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::editorCopy()
{
    step(QStringLiteral("checks: Copy puts the editor text on the clipboard"));
    m_model->openEditor();
    XRAY_LOAD("PageProtocolXrayConfigEditor");
    QGuiApplication::clipboard()->clear();
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);

    click(button(QStringLiteral("Copy")));
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), m_model->inboundText());
    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(), QStringLiteral("Copied"));
}
