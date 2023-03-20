#include "WizardLogic.h"
#include "../uilogic.h"

WizardLogic::WizardLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_radioButtonHighChecked{false},
    m_radioButtonMediumChecked{true},
    m_radioButtonLowChecked{false},
    m_lineEditHighWebsiteMaskingText{},
    m_checkBoxVpnModeChecked{false}
{

}

void WizardLogic::onUpdatePage()
{
    set_lineEditHighWebsiteMaskingText(protocols::cloak::defaultRedirSite);
    set_radioButtonMediumChecked(true);
}

QPair<DockerContainer, QJsonObject> WizardLogic::getInstallConfigsFromWizardPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, ContainerProps::containerToString(DockerContainer::Cloak) },
        { ProtocolProps::protoToString(Proto::Cloak), QJsonObject {
                { config_key::site, lineEditHighWebsiteMaskingText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, ContainerProps::containerToString(DockerContainer::ShadowSocks) }
    };
    QJsonObject openVpnConfig {
        { config_key::container, ContainerProps::containerToString(DockerContainer::OpenVpn) }
    };

    QPair<DockerContainer, QJsonObject> container;

    DockerContainer dockerContainer;

    if (radioButtonHighChecked()) {
        container = {DockerContainer::Cloak, cloakConfig};
    }

    if (radioButtonMediumChecked()) {
        container = {DockerContainer::ShadowSocks, ssConfig};
    }

    if (radioButtonLowChecked()) {
        container = {DockerContainer::OpenVpn, openVpnConfig};
    }

    return container;
}

void WizardLogic::onPushButtonVpnModeFinishClicked()
{
    auto container = getInstallConfigsFromWizardPage();
    uiLogic()->installServer(container);
    if (checkBoxVpnModeChecked()) {
        m_settings->setRouteMode(Settings::VpnOnlyForwardSites);
    } else {
        m_settings->setRouteMode(Settings::VpnAllSites);
    }
}

void WizardLogic::onPushButtonLowFinishClicked()
{
    auto container = getInstallConfigsFromWizardPage();
    uiLogic()->installServer(container);
}
