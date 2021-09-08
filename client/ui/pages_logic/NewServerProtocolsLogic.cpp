#include "NewServerProtocolsLogic.h"
#include "../uilogic.h"

NewServerProtocolsLogic::NewServerProtocolsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_pushButtonSettingsCloakChecked{false},
    m_pushButtonSettingsSsChecked{false},
    m_pushButtonSettingsOpenvpnChecked{false},
    m_lineEditCloakPortText{},
    m_lineEditCloakSiteText{},
    m_lineEditSsPortText{},
    m_comboBoxSsCipherText{"chacha20-ietf-poly1305"},
    m_lineEditOpenvpnPortText{},
    m_comboBoxOpenvpnProtoText{"udp"},
    m_frameSettingsParentWireguardVisible{false},
    m_checkBoxCloakChecked{true},
    m_checkBoxSsChecked{false},
    m_checkBoxOpenVpnChecked{false},
    m_progressBarConnectionMinimum{0},
    m_progressBarConnectionMaximum{100}
{
    set_frameSettingsParentWireguardVisible(false);

    connect(this, &NewServerProtocolsLogic::pushButtonConfigureClicked, this, [this](){
        uiLogic()->installServer(getInstallConfigsFromProtocolsPage());
    });
}


void NewServerProtocolsLogic::updatePage()
{
    set_progressBarConnectionMinimum(0);
    set_progressBarConnectionMaximum(300);

    set_pushButtonSettingsCloakChecked(true);
    set_pushButtonSettingsCloakChecked(false);
    set_pushButtonSettingsSsChecked(true);
    set_pushButtonSettingsSsChecked(false);
    set_lineEditCloakPortText(amnezia::protocols::cloak::defaultPort);
    set_lineEditCloakSiteText(amnezia::protocols::cloak::defaultRedirSite);
    set_lineEditSsPortText(amnezia::protocols::shadowsocks::defaultPort);
    set_comboBoxSsCipherText(amnezia::protocols::shadowsocks::defaultCipher);
    set_lineEditOpenvpnPortText(amnezia::protocols::openvpn::defaultPort);
    set_comboBoxOpenvpnProtoText(amnezia::protocols::openvpn::defaultTransportProto);
}

QMap<DockerContainer, QJsonObject> NewServerProtocolsLogic::getInstallConfigsFromProtocolsPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::port, lineEditCloakPortText() },
                { config_key::site, lineEditCloakSiteText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) },
        { config_key::shadowsocks, QJsonObject {
                { config_key::port, lineEditSsPortText() },
                { config_key::cipher, comboBoxSsCipherText() }}
        }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) },
        { config_key::openvpn, QJsonObject {
                { config_key::port, lineEditOpenvpnPortText() },
                { config_key::transport_proto, comboBoxOpenvpnProtoText() }}
        }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (checkBoxCloakChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (checkBoxSsChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (checkBoxOpenVpnChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

