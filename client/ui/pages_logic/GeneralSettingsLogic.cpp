#include "GeneralSettingsLogic.h"
#include "ShareConnectionLogic.h"

#include "../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

GeneralSettingsLogic::GeneralSettingsLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic)
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
    m_uiLogic->selectedServerIndex = m_settings.defaultServerIndex();
    m_uiLogic->goToPage(Page::ServerSettings);
}

void GeneralSettingsLogic::onPushButtonGeneralSettingsShareConnectionClicked()
{
    m_uiLogic->selectedServerIndex = m_settings.defaultServerIndex();
    m_uiLogic->selectedDockerContainer = m_settings.defaultContainer(m_uiLogic->selectedServerIndex);

    m_uiLogic->shareConnectionLogic()->updateSharingPage(m_uiLogic->selectedServerIndex, m_settings.serverCredentials(m_uiLogic->selectedServerIndex), m_uiLogic->selectedDockerContainer);
    m_uiLogic->goToPage(Page::ShareConnection);
}
