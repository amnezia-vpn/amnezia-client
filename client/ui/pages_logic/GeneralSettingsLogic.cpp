#include "GeneralSettingsLogic.h"
#include "ShareConnectionLogic.h"

#include "../uilogic.h"
#include "../models/protocols_model.h"

GeneralSettingsLogic::GeneralSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void GeneralSettingsLogic::onUpdatePage()
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
    qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel())->setSelectedServerIndex(uiLogic()->selectedServerIndex);
    qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel())->setSelectedDockerContainer(uiLogic()->selectedDockerContainer);

    //uiLogic()->shareConnectionLogic()->updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer);
    uiLogic()->goToPage(Page::ShareConnection);
}
