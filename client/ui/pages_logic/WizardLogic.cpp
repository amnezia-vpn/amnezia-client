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

bool WizardLogic::getRadioButtonSetupWizardMediumChecked() const
{
    return m_radioButtonSetupWizardMediumChecked;
}

void WizardLogic::setRadioButtonSetupWizardMediumChecked(bool radioButtonSetupWizardMediumChecked)
{
    if (m_radioButtonSetupWizardMediumChecked != radioButtonSetupWizardMediumChecked) {
        m_radioButtonSetupWizardMediumChecked = radioButtonSetupWizardMediumChecked;
        emit radioButtonSetupWizardMediumCheckedChanged();
    }
}

void WizardLogic::updateWizardHighPage()
{
    setLineEditSetupWizardHighWebsiteMaskingText(protocols::cloak::defaultRedirSite);
}

QString WizardLogic::getLineEditSetupWizardHighWebsiteMaskingText() const
{
    return m_lineEditSetupWizardHighWebsiteMaskingText;
}

void WizardLogic::setLineEditSetupWizardHighWebsiteMaskingText(const QString &lineEditSetupWizardHighWebsiteMaskingText)
{
    if (m_lineEditSetupWizardHighWebsiteMaskingText != lineEditSetupWizardHighWebsiteMaskingText) {
        m_lineEditSetupWizardHighWebsiteMaskingText = lineEditSetupWizardHighWebsiteMaskingText;
        emit lineEditSetupWizardHighWebsiteMaskingTextChanged();
    }
}

bool WizardLogic::getRadioButtonSetupWizardHighChecked() const
{
    return m_radioButtonSetupWizardHighChecked;
}

void WizardLogic::setRadioButtonSetupWizardHighChecked(bool radioButtonSetupWizardHighChecked)
{
    if (m_radioButtonSetupWizardHighChecked != radioButtonSetupWizardHighChecked) {
        m_radioButtonSetupWizardHighChecked = radioButtonSetupWizardHighChecked;
        emit radioButtonSetupWizardHighCheckedChanged();
    }
}

bool WizardLogic::getRadioButtonSetupWizardLowChecked() const
{
    return m_radioButtonSetupWizardLowChecked;
}

void WizardLogic::setRadioButtonSetupWizardLowChecked(bool radioButtonSetupWizardLowChecked)
{
    if (m_radioButtonSetupWizardLowChecked != radioButtonSetupWizardLowChecked) {
        m_radioButtonSetupWizardLowChecked = radioButtonSetupWizardLowChecked;
        emit radioButtonSetupWizardLowCheckedChanged();
    }
}

bool WizardLogic::getCheckBoxSetupWizardVpnModeChecked() const
{
    return m_checkBoxSetupWizardVpnModeChecked;
}

void WizardLogic::setCheckBoxSetupWizardVpnModeChecked(bool checkBoxSetupWizardVpnModeChecked)
{
    if (m_checkBoxSetupWizardVpnModeChecked != checkBoxSetupWizardVpnModeChecked) {
        m_checkBoxSetupWizardVpnModeChecked = checkBoxSetupWizardVpnModeChecked;
        emit checkBoxSetupWizardVpnModeCheckedChanged();
    }
}

QMap<DockerContainer, QJsonObject> WizardLogic::getInstallConfigsFromWizardPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::site, getLineEditSetupWizardHighWebsiteMaskingText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (getRadioButtonSetupWizardHighChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (getRadioButtonSetupWizardMediumChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (getRadioButtonSetupWizardLowChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

void WizardLogic::onPushButtonSetupWizardVpnModeFinishClicked()
{
    uiLogic()->installServer(getInstallConfigsFromWizardPage());
    if (getCheckBoxSetupWizardVpnModeChecked()) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    } else {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void WizardLogic::onPushButtonSetupWizardLowFinishClicked()
{
    uiLogic()->installServer(getInstallConfigsFromWizardPage());
}
