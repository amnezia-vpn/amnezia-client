#include "AppSettingsLogic.h"

#include "debug.h"
#include "defines.h"
#include "ui/qautostart.h"

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
    set_checkBoxAutoConnectChecked(m_settings.isAutoConnect());
    set_checkBoxStartMinimizedChecked(m_settings.isStartMinimized());
    set_checkBoxSaveLogsChecked(m_settings.isSaveLogs());

    QString ver = QString("%1: %2 (%3)")
            .arg(tr("Software version"))
            .arg(QString(APP_MAJOR_VERSION))
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
    m_settings.setAutoConnect(checked);
}

void AppSettingsLogic::onCheckBoxStartMinimizedToggled(bool checked)
{
    m_settings.setStartMinimized(checked);
}

void AppSettingsLogic::onCheckBoxSaveLogsCheckedToggled(bool checked)
{
    m_settings.setSaveLogs(checked);
}

void AppSettingsLogic::onPushButtonOpenLogsClicked()
{
    Debug::openLogsFolder();
}

void AppSettingsLogic::onPushButtonExportLogsClicked()
{
    QString log = Debug::getLogFile();
    QString ext = ".log";

    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save log"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*" + ext);

    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(ext)) fileName.append(ext);

    QFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(log.toUtf8());
    save.close();

    QFileInfo fi(fileName);
    QDesktopServices::openUrl(fi.absoluteDir().absolutePath());
}

void AppSettingsLogic::onPushButtonClearLogsClicked()
{
    Debug::clearLogs();
    Debug::clearServiceLogs();
}
