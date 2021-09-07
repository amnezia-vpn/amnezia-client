#include "GeneralSettingsLogic.h"
#include "ShareConnectionLogic.h"

#include "../uilogic.h"

GeneralSettingsLogic::GeneralSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void GeneralSettingsLogic::updateGeneralSettingPage()
{
    set_pushButtonGeneralSettingsShareConnectionEnable(m_settings.haveAuthData(m_settings.defaultServerIndex()));
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
