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
    uiLogic()->m_selectedServerIndex = m_settings->defaultServerIndex();
    set_existsAnyServer(uiLogic()->m_selectedServerIndex >= 0);
    uiLogic()->m_selectedDockerContainer = m_settings->defaultContainer(m_settings->defaultServerIndex());

    set_pushButtonGeneralSettingsShareConnectionEnable(m_settings->haveAuthData(m_settings->defaultServerIndex()));
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsServerSettingsClicked()
{
    uiLogic()->m_selectedServerIndex = m_settings->defaultServerIndex();
    uiLogic()->m_selectedDockerContainer = m_settings->defaultContainer(m_settings->defaultServerIndex());

    emit uiLogic()->goToPage(Page::ServerSettings);
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsShareConnectionClicked()
{
    uiLogic()->m_selectedServerIndex = m_settings->defaultServerIndex();
    uiLogic()->m_selectedDockerContainer = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);

    qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel())->setSelectedServerIndex(uiLogic()->m_selectedServerIndex);
    qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel())->setSelectedDockerContainer(uiLogic()->m_selectedDockerContainer);

    uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer);
    emit uiLogic()->goToPage(Page::ShareConnection);
}
