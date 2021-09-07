#include "GeneralSettingsLogic.h"
#include "ShareConnectionLogic.h"

#include "../uilogic.h"

GeneralSettingsLogic::GeneralSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void GeneralSettingsLogic::updateGeneralSettingPage()
{
    setPushButtonGeneralSettingsShareConnectionEnable(m_settings.haveAuthData(m_settings.defaultServerIndex()));
}

bool GeneralSettingsLogic::getPushButtonGeneralSettingsShareConnectionEnable() const
{
    return m_pushButtonGeneralSettingsShareConnectionEnable;
}

void GeneralSettingsLogic::setPushButtonGeneralSettingsShareConnectionEnable(bool pushButtonGeneralSettingsShareConnectionEnable)
{
    if (m_pushButtonGeneralSettingsShareConnectionEnable != pushButtonGeneralSettingsShareConnectionEnable) {
        m_pushButtonGeneralSettingsShareConnectionEnable = pushButtonGeneralSettingsShareConnectionEnable;
        emit pushButtonGeneralSettingsShareConnectionEnableChanged();
    }
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsServerSettingsClicked()
{
    uiLogic()->selectedServerIndex = m_settings.defaultServerIndex();
    uiLogic()->goToPage(Page::ServerSettings);
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsShareConnectionClicked()
{
    uiLogic()->selectedServerIndex = m_settings.defaultServerIndex();
    uiLogic()->selectedDockerContainer = m_settings.defaultContainer(uiLogic()->selectedServerIndex);

    uiLogic()->shareConnectionLogic()->updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer);
    uiLogic()->goToPage(Page::ShareConnection);
}
