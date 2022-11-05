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
    uiLogic()->selectedServerIndex = m_settings->defaultServerIndex();
	set_existsAnyServer(uiLogic()->selectedServerIndex >= 0);
    uiLogic()->selectedDockerContainer = m_settings->defaultContainer(m_settings->defaultServerIndex());

    set_pushButtonGeneralSettingsShareConnectionEnable(m_settings->haveAuthData(m_settings->defaultServerIndex()));
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsServerSettingsClicked()
{
    uiLogic()->selectedServerIndex = m_settings->defaultServerIndex();
    uiLogic()->selectedDockerContainer = m_settings->defaultContainer(m_settings->defaultServerIndex());

    emit uiLogic()->goToPage(Page::ServerSettings);
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsShareConnectionClicked()
{
    uiLogic()->selectedServerIndex = m_settings->defaultServerIndex();
    uiLogic()->selectedDockerContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);

    qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel())->setSelectedServerIndex(uiLogic()->selectedServerIndex);
    qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel())->setSelectedDockerContainer(uiLogic()->selectedDockerContainer);

    uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    emit uiLogic()->goToPage(Page::ShareConnection);
}
