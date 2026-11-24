#include <QRegularExpression>

#include "testXrayPagesUi.h"
#include "ui/utils/pageEnum.h"

using namespace amnezia;

namespace
{
    QQuickItem *rowOf(QQuickItem *item)
    {
        QQuickItem *p = item;
        while (p && !QString::fromLatin1(p->metaObject()->className()).startsWith(QLatin1String("RadioItemType")))
            p = p->parentItem();
        return p;
    }
}

void TestXrayPagesUi::presetsBuiltinList()
{
    step(QStringLiteral("checks: five built-in presets in order, only the current one checked, only the recommended "
                        "one has a badge"));
    XRAY_LOAD("PageProtocolXrayPresets");

    QStringList shown;
    QStringList checked;
    for (QQuickItem *candidate : visibleOfType("RadioItemType")) {
        if (!candidate->property("builtin").toBool())
            continue;
        shown << candidate->property("text").toString();
        if (candidate->property("checked").toBool())
            checked << candidate->property("text").toString();
    }
    QStringList expected;
    for (const XrayPreset &preset : xrayPresets::builtins(QStringLiteral("www.example.com"), QStringLiteral("443")))
        expected << preset.name;
    QCOMPARE(shown, expected);
    QCOMPARE(checked, QStringList { builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)) });

    const QList<QQuickItem *> badges = visibleOfType("RecommendationBadgeType");
    QCOMPARE(badges.size(), 1);
    QCOMPARE(rowOf(badges.first())->property("text").toString(),
             builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)));
}

void TestXrayPagesUi::presetsBuiltinClickSelects()
{
    step(QStringLiteral("checks: clicking a built-in preset selects it in the model and moves the check"));
    XRAY_LOAD("PageProtocolXrayPresets");
    const QString name = builtinName(QStringLiteral("tls-raw-vision"));
    QQuickItem *presetRow = radio(name);
    QVERIFY(presetRow);
    click(presetRow);

    QCOMPARE(m_model->presetId(), QStringLiteral("tls-raw-vision"));
    QVERIFY(m_model->hasUnsavedChanges());
    QVERIFY(presetRow->property("checked").toBool());
    QVERIFY(!radio(builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)))->property("checked").toBool());
}

void TestXrayPagesUi::presetsCustomGroupHiddenWhenEmpty()
{
    step(QStringLiteral("checks: Custom group is hidden without custom presets"));
    XRAY_LOAD("PageProtocolXrayPresets");
    QCOMPARE(m_presets->customCount(), 0);
    QQuickItem *header = typed("SmallTextType", "text", QStringLiteral("Custom"));
    QVERIFY(header);
    QVERIFY(!header->isVisible());
}

void TestXrayPagesUi::presetsCustomListed()
{
    step(QStringLiteral("checks: custom preset is listed with its creation date and a menu button"));
    m_presets->addPreset(QStringLiteral("Office"), m_model->inboundText());
    XRAY_LOAD("PageProtocolXrayPresets");

    QVERIFY(typed("SmallTextType", "text", QStringLiteral("Custom"))->isVisible());
    QQuickItem *presetRow = radio(QStringLiteral("Office"));
    QVERIFY(presetRow);
    QVERIFY(presetRow->isVisible());
    QVERIFY(!presetRow->property("checked").toBool());
    const QString createdAt = presetRow->property("subtitle").toString();
    QVERIFY2(QRegularExpression(QStringLiteral("^\\d{2}\\.\\d{2}\\.\\d{4} \\d{2}:\\d{2}$")).match(createdAt).hasMatch(),
             qPrintable(createdAt));
    QVERIFY(iconButton(QStringLiteral("more-vertical.svg"), presetRow));
}

void TestXrayPagesUi::presetsCustomClickSelects()
{
    step(QStringLiteral("checks: clicking a custom preset applies its JSON"));
    const QString id = m_presets->addPreset(
            QStringLiteral("Office"), editedInbound(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443")));
    XRAY_LOAD("PageProtocolXrayPresets");
    QQuickItem *presetRow = radio(QStringLiteral("Office"));
    QVERIFY(presetRow);
    click(presetRow);

    QCOMPARE(m_model->presetId(), id);
    QCOMPARE(m_model->port(), 8443);
    QVERIFY(presetRow->property("checked").toBool());
}

void TestXrayPagesUi::presetsMenuButtonDoesNotSelect()
{
    step(QStringLiteral("checks: the menu button opens the menu without selecting the preset"));
    m_presets->addPreset(QStringLiteral("Office"), m_model->inboundText());
    XRAY_LOAD("PageProtocolXrayPresets");
    const QString selected = m_model->presetId();
    QQuickItem *presetRow = radio(QStringLiteral("Office"));
    QVERIFY(presetRow);
    click(iconButton(QStringLiteral("more-vertical.svg"), presetRow));

    QObject *menu = popup("PopupMenuType");
    QVERIFY(waitOpened(menu));
    QCOMPARE(m_model->presetId(), selected);
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(!presetRow->property("checked").toBool());
}

void TestXrayPagesUi::presetsMenuShareNavigates()
{
    step(QStringLiteral("checks: Share preset in the menu opens the share page for that preset"));
    const QString id = m_presets->addPreset(QStringLiteral("Office"), m_model->inboundText());
    XRAY_LOAD("PageProtocolXrayPresets");
    click(iconButton(QStringLiteral("more-vertical.svg"), radio(QStringLiteral("Office"))));
    QObject *menu = popup("PopupMenuType");
    QVERIFY(waitOpened(menu));

    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(typed("PopupMenuItemType", "text", QStringLiteral("Share preset"), popupContent(menu)));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)), PageLoader::PageEnum::PageProtocolXraySharePreset);
    QCOMPARE(pages.first().at(2).toMap().value(QStringLiteral("presetId")).toString(), id);
    QVERIFY(waitClosed(menu));
}

void TestXrayPagesUi::presetsDeleteCancelled()
{
    step(QStringLiteral("checks: Delete preset dialog texts, Cancel keeps the preset"));
    m_presets->addPreset(QStringLiteral("Office"), m_model->inboundText());
    XRAY_LOAD("PageProtocolXrayPresets");
    click(iconButton(QStringLiteral("more-vertical.svg"), radio(QStringLiteral("Office"))));
    QObject *menu = popup("PopupMenuType");
    QVERIFY(waitOpened(menu));
    click(typed("PopupMenuItemType", "text", QStringLiteral("Delete preset"), popupContent(menu)));

    QObject *dialog = alert(QStringLiteral("Delete preset?"));
    QVERIFY(waitOpened(dialog));
    QCOMPARE(dialog->property("descriptionText").toString(),
             QStringLiteral("“Office” will be removed from the list. Server settings stay as they are"));
    QCOMPARE(dialog->property("confirmButtonText").toString(), QStringLiteral("Delete"));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Cancel")));

    QCOMPARE(m_presets->customCount(), 1);
}

void TestXrayPagesUi::presetsDeleteConfirmed()
{
    step(QStringLiteral("checks: confirmed Delete removes the preset from the list and the storage"));
    m_presets->addPreset(QStringLiteral("Office"), m_model->inboundText());
    XRAY_LOAD("PageProtocolXrayPresets");
    click(iconButton(QStringLiteral("more-vertical.svg"), radio(QStringLiteral("Office"))));
    QObject *menu = popup("PopupMenuType");
    QVERIFY(waitOpened(menu));
    click(typed("PopupMenuItemType", "text", QStringLiteral("Delete preset"), popupContent(menu)));
    QObject *dialog = alert(QStringLiteral("Delete preset?"));
    QVERIFY(waitOpened(dialog));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Delete")));

    QCOMPARE(m_presets->customCount(), 0);
    QVERIFY(xrayPresets::customFromStorage(m_core->m_appSettingsRepository->xrayPresets()).isEmpty());
    QVERIFY(!typed("SmallTextType", "text", QStringLiteral("Custom"))->isVisible());
}

void TestXrayPagesUi::presetsDeleteSelectedFallsBackToBuiltin()
{
    step(QStringLiteral("checks: deleting the selected custom preset selects the matching built-in one and disables "
                        "Save"));
    m_model->saveAsPreset(QStringLiteral("Office"));
    QVERIFY(m_model->hasUnsavedChanges());
    XRAY_LOAD("PageProtocolXrayPresets");
    click(iconButton(QStringLiteral("more-vertical.svg"), radio(QStringLiteral("Office"))));
    QObject *menu = popup("PopupMenuType");
    QVERIFY(waitOpened(menu));
    click(typed("PopupMenuItemType", "text", QStringLiteral("Delete preset"), popupContent(menu)));
    QObject *dialog = alert(QStringLiteral("Delete preset?"));
    QVERIFY(waitOpened(dialog));
    QVERIFY(clickDialogButton(dialog, QStringLiteral("Delete")));

    QCOMPARE(m_model->presetId(), QString::fromLatin1(xrayPresets::defaultPresetId));
    QCOMPARE(m_model->presetName(), builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)));
    QVERIFY(!m_model->hasUnsavedChanges());
    QVERIFY(radio(builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)))->property("checked").toBool());
}

void TestXrayPagesUi::presetsImportRowNavigates()
{
    step(QStringLiteral("checks: Import preset row opens the import page"));
    XRAY_LOAD("PageProtocolXrayPresets");
    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(row(QStringLiteral("Import preset")));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)),
             PageLoader::PageEnum::PageProtocolXrayImportPreset);
}

void TestXrayPagesUi::presetsShareCurrentNavigates()
{
    step(QStringLiteral("checks: Share current preset opens the share page without a preset id"));
    XRAY_LOAD("PageProtocolXrayPresets");
    QSignalSpy pages(m_pages, &PageController::goToPage);
    click(row(QStringLiteral("Share current preset")));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(qvariant_cast<PageLoader::PageEnum>(pages.first().at(0)), PageLoader::PageEnum::PageProtocolXraySharePreset);
    QCOMPARE(pages.first().at(2).toMap().value(QStringLiteral("presetId")).toString(), QString());
}

void TestXrayPagesUi::presetsSaveAsSheet()
{
    step(QStringLiteral("checks: Save as preset sheet prefills the name and stores a custom preset"));
    XRAY_LOAD("PageProtocolXrayPresets");
    QSignalSpy notes(m_pages, &PageController::showNotificationMessage);
    click(row(QStringLiteral("Save as preset")));
    QObject *sheet = popup("XraySavePresetSheet");
    QVERIFY(waitOpened(sheet));
    QTest::qWait(300);

    QQuickItem *name = field(QStringLiteral("Name"), popupContent(sheet));
    QVERIFY(name);
    QCOMPARE(name->property("text").toString(), builtinName(QString::fromLatin1(xrayPresets::defaultPresetId)));
    clearAndType(name, QStringLiteral("Office"));
    click(button(QStringLiteral("Save"), popupContent(sheet)));
    QVERIFY(waitClosed(sheet));

    const QStringList ids = customIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(m_presets->presetName(ids.first()), QStringLiteral("Office"));
    QCOMPARE(xrayInbound::normalized(m_presets->presetJson(ids.first())),
             xrayInbound::normalized(m_model->inboundText()));
    QCOMPARE(xrayPresets::customFromStorage(m_core->m_appSettingsRepository->xrayPresets()).size(), 1);
    QCOMPARE(notes.count(), 1);
    QCOMPARE(notes.first().at(0).toString(), QStringLiteral("Preset saved"));
}

void TestXrayPagesUi::presetsSaveAsSheetNeedsName()
{
    step(QStringLiteral("checks: Save as preset sheet disables Save for a blank name"));
    XRAY_LOAD("PageProtocolXrayPresets");
    click(row(QStringLiteral("Save as preset")));
    QObject *sheet = popup("XraySavePresetSheet");
    QVERIFY(waitOpened(sheet));
    QTest::qWait(300);

    QQuickItem *save = button(QStringLiteral("Save"), popupContent(sheet));
    QVERIFY(save);
    QVERIFY(save->isEnabled());
    clearAndType(field(QStringLiteral("Name"), popupContent(sheet)), QStringLiteral("   "));
    QVERIFY(!save->isEnabled());
    QCOMPARE(m_presets->customCount(), 0);
}
