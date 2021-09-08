#include "WizardLogic.h"
#include "../uilogic.h"

WizardLogic::WizardLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_radioButtonSetupWizardHighChecked{false},
    m_radioButtonSetupWizardMediumChecked{true},
    m_radioButtonSetupWizardLowChecked{false},
    m_lineEditSetupWizardHighWebsiteMaskingText{},
    m_checkBoxSetupWizardVpnModeChecked{false}
{

}

void WizardLogic::updateWizardHighPage()
{
    set_lineEditSetupWizardHighWebsiteMaskingText(protocols::cloak::defaultRedirSite);
}

QMap<DockerContainer, QJsonObject> WizardLogic::getInstallConfigsFromWizardPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::site, lineEditSetupWizardHighWebsiteMaskingText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (radioButtonSetupWizardHighChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (radioButtonSetupWizardMediumChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (radioButtonSetupWizardLowChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

void WizardLogic::onPushButtonSetupWizardVpnModeFinishClicked()
{
    uiLogic()->installServer(getInstallConfigsFromWizardPage());
    if (checkBoxSetupWizardVpnModeChecked()) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    } else {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void WizardLogic::onPushButtonSetupWizardLowFinishClicked()
{
    uiLogic()->installServer(getInstallConfigsFromWizardPage());
}
