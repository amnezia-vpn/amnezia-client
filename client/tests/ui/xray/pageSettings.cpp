#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"

using namespace amnezia;

namespace
{
    const QString kSaveText = QStringLiteral(
            "The server configuration will be updated. Everyone you shared this connection with will lose access. "
            "Share it again after saving");
}

void TestXrayPagesUi::settingsShowsMatchedPreset()
{
    step(QStringLiteral("checks: stored REALITY RAW Vision server is recognised as the built-in preset, subtitles show "
                        "it and port 443, Save is disabled"));
    XRAY_LOAD("PageProtocolXraySettings");

    QCOMPARE(m_model->presetId(), QString::fromLatin1(xrayPresets::defaultPresetId));
    QVERIFY(!m_model->presetEdited());
    QCOMPARE(row(QStringLiteral("Preset"))->property("subtitle").toString(),
             builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)));
    QCOMPARE(row(QStringLiteral("Edit config"))->property("subtitle").toString(),
             QStringLiteral("server.json · port 443"));
    QVERIFY(!button(QStringLiteral("Save"))->isEnabled());
}

void TestXrayPagesUi::settingsUnmatchedConfigIsCustom()
{
    step(QStringLiteral("checks: config without a matching built-in preset shows \"Custom config\""));
    storeAndOpen(xraytest::baseConfig(QStringLiteral("mkcp"), QStringLiteral("none"), QString()));
    XRAY_LOAD("PageProtocolXraySettings");

    QCOMPARE(m_model->presetId(), QString());
    QCOMPARE(row(QStringLiteral("Preset"))->property("subtitle").toString(), QStringLiteral("Custom config"));
    QVERIFY(!button(QStringLiteral("Save"))->isEnabled());
}

void TestXrayPagesUi::settingsPresetRowNavigates()
{
    step(QStringLiteral("checks: Preset row opens the presets page"));
    XRAY_LOAD("PageProtocolXraySettings");
    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(row(QStringLiteral("Preset")));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)), PageLoader::PageEnum::PageProtocolXrayPresets);
}

void TestXrayPagesUi::settingsEditRowOpensEditor()
{
    step(QStringLiteral("checks: Edit config row resets the draft and opens the editor"));
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setDraft(QStringLiteral("{"));
    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(row(QStringLiteral("Edit config")));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)),
             PageLoader::PageEnum::PageProtocolXrayConfigEditor);
    QCOMPARE(m_model->draftText(), m_model->inboundText());
    QVERIFY(!m_model->draftModified());
}

void TestXrayPagesUi::settingsPresetChangeTogglesSave()
{
    step(QStringLiteral("checks: choosing another preset enables Save, choosing the original one disables it"));
    XRAY_LOAD("PageProtocolXraySettings");
    QQuickItem *save = button(QStringLiteral("Save"));
    QQuickItem *preset = row(QStringLiteral("Preset"));
    QVERIFY(save && preset);

    m_model->selectPreset(QStringLiteral("tls-raw-vision"));
    QVERIFY(save->isEnabled());
    QCOMPARE(preset->property("subtitle").toString(), builtinName(QStringLiteral("tls-raw-vision")));

    m_model->selectPreset(QString::fromLatin1(xrayPresets::defaultPresetId));
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!save->isEnabled());
}

void TestXrayPagesUi::settingsEditedPresetSubtitle()
{
    step(QStringLiteral("checks: committed JSON edit marks the preset as edited and shows the new port"));
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setDraft(editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    QVERIFY(m_model->commitDraft());

    QVERIFY(m_model->presetEdited());
    QCOMPARE(row(QStringLiteral("Preset"))->property("subtitle").toString(),
             builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)) + QStringLiteral(" · edited"));
    QCOMPARE(row(QStringLiteral("Edit config"))->property("subtitle").toString(),
             QStringLiteral("server.json · port 8443"));
    QVERIFY(button(QStringLiteral("Save"))->isEnabled());
}

void TestXrayPagesUi::settingsSaveDialogCancelled()
{
    step(QStringLiteral("checks: Save opens the confirmation dialog, Cancel neither saves nor navigates"));
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->selectPreset(QStringLiteral("tls-raw-vision"));
    QSignalSpy pages(m_pages, &PageController::goToPage);
    QSignalSpy errors(m_pages, qOverload<const QString &>(&PageController::showErrorMessage));
    QSignalSpy finished(m_core->m_installUiController, &InstallUiController::updateContainerFinished);
    click(button(QStringLiteral("Save")));

    QObject *dialog = alert(QStringLiteral("Save settings?"));
    QVERIFY(waitOpened(dialog));
    QCOMPARE(dialog->property("descriptionText").toString(), kSaveText);
    QCOMPARE(dialog->property("confirmButtonText").toString(), QStringLiteral("Save"));
    QCOMPARE(errors.count(), 0);

    QVERIFY(clickDialogButton(dialog, QStringLiteral("Cancel")));
    QTest::qWait(300);
    QCOMPARE(pages.count(), 0);
    QCOMPARE(finished.count(), 0);
    QVERIFY(m_model->hasUnsavedChanges());
    QCOMPARE(stored().security, QStringLiteral("reality"));
}

void TestXrayPagesUi::settingsResetDialogTexts()
{
    step(QStringLiteral("checks: Reset settings opens the dialog with the design texts"));
    XRAY_LOAD("PageProtocolXraySettings");
    click(button(QStringLiteral("Reset settings")));

    QObject *dialog = alert(QStringLiteral("Reset settings?"));
    QVERIFY(waitOpened(dialog));
    QCOMPARE(dialog->property("descriptionText").toString(),
             QStringLiteral("All XRay settings will be restored to defaults"));
    QCOMPARE(dialog->property("confirmButtonText").toString(), QStringLiteral("Reset"));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Cancel")));
}

void TestXrayPagesUi::settingsResetAccepted()
{
    step(QStringLiteral("checks: Reset restores the preset JSON and clears unsaved changes"));
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setDraft(editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    QVERIFY(m_model->commitDraft());
    QCOMPARE(m_model->port(), 8443);

    click(button(QStringLiteral("Reset settings")));
    QObject *dialog = alert(QStringLiteral("Reset settings?"));
    QVERIFY(waitOpened(dialog));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Reset")));

    QCOMPARE(m_model->port(), 443);
    QVERIFY(!m_model->presetEdited());
    QVERIFY(!m_model->hasUnsavedChanges());
    QCOMPARE(row(QStringLiteral("Preset"))->property("subtitle").toString(),
             builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)));
    QVERIFY(!button(QStringLiteral("Save"))->isEnabled());
}

void TestXrayPagesUi::settingsResetCancelled()
{
    step(QStringLiteral("checks: cancelled Reset keeps the edited config"));
    XRAY_LOAD("PageProtocolXraySettings");
    m_model->setDraft(editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    QVERIFY(m_model->commitDraft());

    click(button(QStringLiteral("Reset settings")));
    QObject *dialog = alert(QStringLiteral("Reset settings?"));
    QVERIFY(waitOpened(dialog));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Cancel")));

    QCOMPARE(m_model->port(), 8443);
    QVERIFY(m_model->presetEdited());
    QVERIFY(m_model->hasUnsavedChanges());
}

void TestXrayPagesUi::settingsKeyMigrationDialog()
{
    step(QStringLiteral("checks: key migration request hides the busy indicator and asks a question"));
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

void TestXrayPagesUi::settingsBackButtonClosesPage()
{
    step(QStringLiteral("checks: back arrow closes the page"));
    XRAY_LOAD("PageProtocolXraySettings");
    QSignalSpy closed(m_pages, &PageController::closePage);
    click(iconButton(QStringLiteral("arrow-left.svg")));
    QCOMPARE(closed.count(), 1);
}
