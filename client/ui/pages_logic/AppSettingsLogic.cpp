#include "AppSettingsLogic.h"

#include "logger.h"
#include "version.h"
#include "ui/qautostart.h"
#include "ui/uilogic.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QStandardPaths>

using namespace amnezia;
using namespace PageEnumNS;

AppSettingsLogic::AppSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_checkBoxAutostartChecked{false},
    m_checkBoxAutoConnectChecked{false},
    m_checkBoxStartMinimizedChecked{false},
    m_checkBoxSaveLogsChecked{false}
{

}

void AppSettingsLogic::onUpdatePage()
{
    set_checkBoxAutostartChecked(Autostart::isAutostart());
    set_checkBoxAutoConnectChecked(m_settings->isAutoConnect());
    set_checkBoxStartMinimizedChecked(m_settings->isStartMinimized());
    set_checkBoxSaveLogsChecked(m_settings->isSaveLogs());

    QString ver = QString("%1: %2 (%3)")
            .arg(tr("Software version"))
            .arg(QString(APP_VERSION))
            .arg(__DATE__);
    set_labelVersionText(ver);
}

void AppSettingsLogic::onCheckBoxAutostartToggled(bool checked)
{
    if (!checked) {
        set_checkBoxAutoConnectChecked(false);
    }
    Autostart::setAutostart(checked);
}

void AppSettingsLogic::onCheckBoxAutoconnectToggled(bool checked)
{
    m_settings->setAutoConnect(checked);
}

void AppSettingsLogic::onCheckBoxStartMinimizedToggled(bool checked)
{
    m_settings->setStartMinimized(checked);
}

void AppSettingsLogic::onCheckBoxSaveLogsCheckedToggled(bool checked)
{
    m_settings->setSaveLogs(checked);
}

void AppSettingsLogic::onPushButtonOpenLogsClicked()
{
    Logger::openLogsFolder();
}

void AppSettingsLogic::onPushButtonExportLogsClicked()
{
    uiLogic()->saveTextFile(tr("Save log"), "AmneziaVPN.log", ".log", Logger::getLogFile());
}

void AppSettingsLogic::onPushButtonClearLogsClicked()
{
    Logger::clearLogs();
    Logger::clearServiceLogs();
}

void AppSettingsLogic::onPushButtonBackupAppConfigClicked()
{
    uiLogic()->saveTextFile("Backup application config", "AmneziaVPN.backup", ".backup", m_settings->backupAppConfig());
}

void AppSettingsLogic::onPushButtonRestoreAppConfigClicked()
{
    QString fileName = QFileDialog::getOpenFileName(Q_NULLPTR, tr("Open backup"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.backup");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();

    bool ok = m_settings->restoreAppConfig(data);
    if (ok) {
        emit uiLogic()->goToPage(Page::Vpn);
        emit uiLogic()->setStartPage(Page::Vpn);
    } else {
        emit uiLogic()->showWarningMessage(tr("Can't import config, file is corrupted."));
    }

}

