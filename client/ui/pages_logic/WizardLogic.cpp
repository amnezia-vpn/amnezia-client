#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QRegularExpression>
#include <QSaveFile>

//#include "configurators/cloak_configurator.h"
//#include "configurators/vpn_configurator.h"
//#include "configurators/openvpn_configurator.h"
//#include "configurators/shadowsocks_configurator.h"
//#include "configurators/ssh_configurator.h"

//#include "core/servercontroller.h"
//#include "core/server_defs.h"
//#include "core/errorstrings.h"

//#include "protocols/protocols_defs.h"
//#include "protocols/shadowsocksvpnprotocol.h"

//#include "debug.h"
//#include "defines.h"
#include "WizardLogic.h"
//#include "utils.h"
//#include "vpnconnection.h"
//#include <functional>

#include "../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;


WizardLogic::WizardLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
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
    m_uiLogic->installServer(getInstallConfigsFromWizardPage());
    if (getCheckBoxSetupWizardVpnModeChecked()) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    } else {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void WizardLogic::onPushButtonSetupWizardLowFinishClicked()
{
    m_uiLogic->installServer(getInstallConfigsFromWizardPage());
}
