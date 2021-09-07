#include "AppSettingsLogic.h"

#include "debug.h"
#include "defines.h"
#include "ui/qautostart.h"

using namespace amnezia;
using namespace PageEnumNS;

AppSettingsLogic::AppSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_checkBoxAutostartChecked{false},
    m_checkBoxAutoConnectChecked{false},
    m_checkBoxStartMinimizedChecked{false}
{

}

void AppSettingsLogic::updatePage()
{
    set_checkBoxAutostartChecked(Autostart::isAutostart());
    set_checkBoxAutoConnectChecked(m_settings.isAutoConnect());
    set_checkBoxStartMinimizedChecked(m_settings.isStartMinimized());

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

void AppSettingsLogic::onPushButtonOpenLogsClicked()
{
    Debug::openLogsFolder();
}
