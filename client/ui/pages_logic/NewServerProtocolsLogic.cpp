#include "NewServerProtocolsLogic.h"

using namespace amnezia;
using namespace PageEnumNS;

NewServerProtocolsLogic::NewServerProtocolsLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_pushButtonNewServerSettingsCloakChecked{false},
    m_pushButtonNewServerSettingsSsChecked{false},
    m_pushButtonNewServerSettingsOpenvpnChecked{false},
    m_lineEditNewServerCloakPortText{},
    m_lineEditNewServerCloakSiteText{},
    m_lineEditNewServerSsPortText{},
    m_comboBoxNewServerSsCipherText{"chacha20-ietf-poly1305"},
    m_lineEditNewServerOpenvpnPortText{},
    m_comboBoxNewServerOpenvpnProtoText{"udp"},
    m_frameNewServerSettingsParentWireguardVisible{false},
    m_checkBoxNewServerCloakChecked{true},
    m_checkBoxNewServerSsChecked{false},
    m_checkBoxNewServerOpenvpnChecked{false},
    m_progressBarNewServerConnectionMinimum{0},
    m_progressBarNewServerConnectionMaximum{100}
{
    setFrameNewServerSettingsParentWireguardVisible(false);

}

bool NewServerProtocolsLogic::getFrameNewServerSettingsParentWireguardVisible() const
{
    return m_frameNewServerSettingsParentWireguardVisible;
}

void NewServerProtocolsLogic::setFrameNewServerSettingsParentWireguardVisible(bool frameNewServerSettingsParentWireguardVisible)
{
    if (m_frameNewServerSettingsParentWireguardVisible != frameNewServerSettingsParentWireguardVisible) {
        m_frameNewServerSettingsParentWireguardVisible = frameNewServerSettingsParentWireguardVisible;
        emit frameNewServerSettingsParentWireguardVisibleChanged();
    }
}



void NewServerProtocolsLogic::updateNewServerProtocolsPage()
{
    setProgressBarNewServerConnectionMinimum(0);
    setProgressBarNewServerConnectionMaximum(300);

    setPushButtonNewServerSettingsCloakChecked(true);
    setPushButtonNewServerSettingsCloakChecked(false);
    setPushButtonNewServerSettingsSsChecked(true);
    setPushButtonNewServerSettingsSsChecked(false);
    setLineEditNewServerCloakPortText(amnezia::protocols::cloak::defaultPort);
    setLineEditNewServerCloakSiteText(amnezia::protocols::cloak::defaultRedirSite);
    setLineEditNewServerSsPortText(amnezia::protocols::shadowsocks::defaultPort);
    setComboBoxNewServerSsCipherText(amnezia::protocols::shadowsocks::defaultCipher);
    setLineEditNewServerOpenvpnPortText(amnezia::protocols::openvpn::defaultPort);
    setComboBoxNewServerOpenvpnProtoText(amnezia::protocols::openvpn::defaultTransportProto);
}



QString NewServerProtocolsLogic::getComboBoxNewServerOpenvpnProtoText() const
{
    return m_comboBoxNewServerOpenvpnProtoText;
}

void NewServerProtocolsLogic::setComboBoxNewServerOpenvpnProtoText(const QString &comboBoxNewServerOpenvpnProtoText)
{
    if (m_comboBoxNewServerOpenvpnProtoText != comboBoxNewServerOpenvpnProtoText) {
        m_comboBoxNewServerOpenvpnProtoText = comboBoxNewServerOpenvpnProtoText;
        emit comboBoxNewServerOpenvpnProtoTextChanged();
    }
}

QString NewServerProtocolsLogic::getLineEditNewServerCloakSiteText() const
{
    return m_lineEditNewServerCloakSiteText;
}

void NewServerProtocolsLogic::setLineEditNewServerCloakSiteText(const QString &lineEditNewServerCloakSiteText)
{
    if (m_lineEditNewServerCloakSiteText != lineEditNewServerCloakSiteText) {
        m_lineEditNewServerCloakSiteText = lineEditNewServerCloakSiteText;
        emit lineEditNewServerCloakSiteTextChanged();
    }
}

QString NewServerProtocolsLogic::getLineEditNewServerSsPortText() const
{
    return m_lineEditNewServerSsPortText;
}

void NewServerProtocolsLogic::setLineEditNewServerSsPortText(const QString &lineEditNewServerSsPortText)
{
    if (m_lineEditNewServerSsPortText != lineEditNewServerSsPortText) {
        m_lineEditNewServerSsPortText = lineEditNewServerSsPortText;
        emit lineEditNewServerSsPortTextChanged();
    }
}

QString NewServerProtocolsLogic::getComboBoxNewServerSsCipherText() const
{
    return m_comboBoxNewServerSsCipherText;
}

void NewServerProtocolsLogic::setComboBoxNewServerSsCipherText(const QString &comboBoxNewServerSsCipherText)
{
    if (m_comboBoxNewServerSsCipherText != comboBoxNewServerSsCipherText) {
        m_comboBoxNewServerSsCipherText = comboBoxNewServerSsCipherText;
        emit comboBoxNewServerSsCipherTextChanged();
    }
}

QString NewServerProtocolsLogic::getlineEditNewServerOpenvpnPortText() const
{
    return m_lineEditNewServerOpenvpnPortText;
}

void NewServerProtocolsLogic::setLineEditNewServerOpenvpnPortText(const QString &lineEditNewServerOpenvpnPortText)
{
    if (m_lineEditNewServerOpenvpnPortText != lineEditNewServerOpenvpnPortText) {
        m_lineEditNewServerOpenvpnPortText = lineEditNewServerOpenvpnPortText;
        emit lineEditNewServerOpenvpnPortTextChanged();
    }
}

bool NewServerProtocolsLogic::getPushButtonNewServerSettingsSsChecked() const
{
    return m_pushButtonNewServerSettingsSsChecked;
}

void NewServerProtocolsLogic::setPushButtonNewServerSettingsSsChecked(bool pushButtonNewServerSettingsSsChecked)
{
    if (m_pushButtonNewServerSettingsSsChecked != pushButtonNewServerSettingsSsChecked) {
        m_pushButtonNewServerSettingsSsChecked = pushButtonNewServerSettingsSsChecked;
        emit pushButtonNewServerSettingsSsCheckedChanged();
    }
}

bool NewServerProtocolsLogic::getPushButtonNewServerSettingsOpenvpnChecked() const
{
    return m_pushButtonNewServerSettingsOpenvpnChecked;
}

void NewServerProtocolsLogic::setPushButtonNewServerSettingsOpenvpnChecked(bool pushButtonNewServerSettingsOpenvpnChecked)
{
    if (m_pushButtonNewServerSettingsOpenvpnChecked != pushButtonNewServerSettingsOpenvpnChecked) {
        m_pushButtonNewServerSettingsOpenvpnChecked = pushButtonNewServerSettingsOpenvpnChecked;
        emit pushButtonNewServerSettingsOpenvpnCheckedChanged();
    }
}

QString NewServerProtocolsLogic::getLineEditNewServerCloakPortText() const
{
    return m_lineEditNewServerCloakPortText;
}

void NewServerProtocolsLogic::setLineEditNewServerCloakPortText(const QString &lineEditNewServerCloakPortText)
{
    if (m_lineEditNewServerCloakPortText != lineEditNewServerCloakPortText) {
        m_lineEditNewServerCloakPortText = lineEditNewServerCloakPortText;
        emit lineEditNewServerCloakPortTextChanged();
    }
}

bool NewServerProtocolsLogic::getPushButtonNewServerSettingsCloakChecked() const
{
    return m_pushButtonNewServerSettingsCloakChecked;
}

void NewServerProtocolsLogic::setPushButtonNewServerSettingsCloakChecked(bool pushButtonNewServerSettingsCloakChecked)
{
    if (m_pushButtonNewServerSettingsCloakChecked != pushButtonNewServerSettingsCloakChecked) {
        m_pushButtonNewServerSettingsCloakChecked = pushButtonNewServerSettingsCloakChecked;
        emit pushButtonNewServerSettingsCloakCheckedChanged();
    }
}

bool NewServerProtocolsLogic::getCheckBoxNewServerCloakChecked() const
{
    return m_checkBoxNewServerCloakChecked;
}

void NewServerProtocolsLogic::setCheckBoxNewServerCloakChecked(bool checkBoxNewServerCloakChecked)
{
    if (m_checkBoxNewServerCloakChecked != checkBoxNewServerCloakChecked) {
        m_checkBoxNewServerCloakChecked = checkBoxNewServerCloakChecked;
        emit checkBoxNewServerCloakCheckedChanged();
    }
}

bool NewServerProtocolsLogic::getCheckBoxNewServerSsChecked() const
{
    return m_checkBoxNewServerSsChecked;
}

void NewServerProtocolsLogic::setCheckBoxNewServerSsChecked(bool checkBoxNewServerSsChecked)
{
    if (m_checkBoxNewServerSsChecked != checkBoxNewServerSsChecked) {
        m_checkBoxNewServerSsChecked = checkBoxNewServerSsChecked;
        emit checkBoxNewServerSsCheckedChanged();
    }
}

bool NewServerProtocolsLogic::getCheckBoxNewServerOpenvpnChecked() const
{
    return m_checkBoxNewServerOpenvpnChecked;
}

void NewServerProtocolsLogic::setCheckBoxNewServerOpenvpnChecked(bool checkBoxNewServerOpenvpnChecked)
{
    if (m_checkBoxNewServerOpenvpnChecked != checkBoxNewServerOpenvpnChecked) {
        m_checkBoxNewServerOpenvpnChecked = checkBoxNewServerOpenvpnChecked;
        emit checkBoxNewServerOpenvpnCheckedChanged();
    }
}

double NewServerProtocolsLogic::getProgressBarNewServerConnectionMinimum() const
{
    return m_progressBarNewServerConnectionMinimum;
}

void NewServerProtocolsLogic::setProgressBarNewServerConnectionMinimum(double progressBarNewServerConnectionMinimum)
{
    if (m_progressBarNewServerConnectionMinimum != progressBarNewServerConnectionMinimum) {
        m_progressBarNewServerConnectionMinimum = progressBarNewServerConnectionMinimum;
        emit progressBarNewServerConnectionMinimumChanged();
    }
}

double NewServerProtocolsLogic::getProgressBarNewServerConnectionMaximum() const
{
    return m_progressBarNewServerConnectionMaximum;
}

void NewServerProtocolsLogic::setProgressBarNewServerConnectionMaximum(double progressBarNewServerConnectionMaximum)
{
    if (m_progressBarNewServerConnectionMaximum != progressBarNewServerConnectionMaximum) {
        m_progressBarNewServerConnectionMaximum = progressBarNewServerConnectionMaximum;
        emit progressBarNewServerConnectionMaximumChanged();
    }
}

QMap<DockerContainer, QJsonObject> NewServerProtocolsLogic::getInstallConfigsFromProtocolsPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::port, getLineEditNewServerCloakPortText() },
                { config_key::site, getLineEditNewServerCloakSiteText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) },
        { config_key::shadowsocks, QJsonObject {
                { config_key::port, getLineEditNewServerSsPortText() },
                { config_key::cipher, getComboBoxNewServerSsCipherText() }}
        }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) },
        { config_key::openvpn, QJsonObject {
                { config_key::port, getlineEditNewServerOpenvpnPortText() },
                { config_key::transport_proto, getComboBoxNewServerOpenvpnProtoText() }}
        }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (getCheckBoxNewServerCloakChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (getCheckBoxNewServerSsChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (getCheckBoxNewServerOpenvpnChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

