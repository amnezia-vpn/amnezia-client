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

void WizardLogic::updatePage()
{
    set_lineEditHighWebsiteMaskingText(protocols::cloak::defaultRedirSite);
}

QMap<DockerContainer, QJsonObject> WizardLogic::getInstallConfigsFromWizardPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, ContainerProps::containerToString(DockerContainer::Cloak) },
        { ProtocolProps::protoToString(Protocol::Cloak), QJsonObject {
                { config_key::site, lineEditHighWebsiteMaskingText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, ContainerProps::containerToString(DockerContainer::ShadowSocks) }
    };
    QJsonObject openVpnConfig {
        { config_key::container, ContainerProps::containerToString(DockerContainer::OpenVpn) }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (radioButtonHighChecked()) {
        containers.insert(DockerContainer::Cloak, cloakConfig);
    }

    if (radioButtonMediumChecked()) {
        containers.insert(DockerContainer::ShadowSocks, ssConfig);
    }

    if (radioButtonLowChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

void WizardLogic::onPushButtonVpnModeFinishClicked()
{
    uiLogic()->installServer(getInstallConfigsFromWizardPage());
    if (checkBoxVpnModeChecked()) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    } else {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void WizardLogic::onPushButtonLowFinishClicked()
{
    uiLogic()->installServer(getInstallConfigsFromWizardPage());
}
