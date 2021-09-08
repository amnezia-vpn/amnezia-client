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
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::site, lineEditHighWebsiteMaskingText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (radioButtonHighChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (radioButtonMediumChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
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
