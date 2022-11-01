#include "OpenVpnLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

OpenVpnLogic::OpenVpnLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent),
    m_lineEditSubnetText{""},

    m_radioButtonTcpEnabled{true},
    m_radioButtonTcpChecked{false},
    m_radioButtonUdpEnabled{true},
    m_radioButtonUdpChecked{false},

    m_checkBoxAutoEncryptionChecked{},
    m_comboBoxVpnCipherText{"AES-256-GCM"},
    m_comboBoxVpnHashText{"SHA512"},
    m_checkBoxBlockDnsChecked{false},
    m_lineEditPortText{},
    m_checkBoxTlsAuthChecked{false},
    m_textAreaAdditionalClientConfig{""},
    m_textAreaAdditionalServerConfig{""},
    m_pushButtonSaveVisible{false},
    m_progressBarResetVisible{false},

    m_lineEditPortEnabled{false},
    m_labelProtoOpenVpnInfoVisible{true},
    m_labelProtoOpenVpnInfoText{},
    m_progressBarResetValue{0},
    m_progressBarResetMaximium{100}
{

}

void OpenVpnLogic::updateProtocolPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData, bool isThirdPartyConfig)
{
    qDebug() << "OpenVpnLogic::updateProtocolPage";
    set_pageEnabled(haveAuthData);
    set_pushButtonSaveVisible(haveAuthData);
    set_progressBarResetVisible(haveAuthData);

    set_radioButtonUdpEnabled(true);
    set_radioButtonTcpEnabled(true);

    set_lineEditSubnetText(openvpnConfig.value(config_key::subnet_address).
                                      toString(protocols::openvpn::defaultSubnetAddress));

    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
            toString(protocols::openvpn::defaultTransportProto);

    set_radioButtonUdpChecked(trasnsport == protocols::openvpn::defaultTransportProto);
    set_radioButtonTcpChecked(trasnsport != protocols::openvpn::defaultTransportProto);

    set_comboBoxVpnCipherText(openvpnConfig.value(config_key::cipher).
                                      toString(protocols::openvpn::defaultCipher));

    set_comboBoxVpnHashText(openvpnConfig.value(config_key::hash).
                                    toString(protocols::openvpn::defaultHash));

    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    set_checkBoxBlockDnsChecked(blockOutsideDns);

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    set_checkBoxAutoEncryptionChecked(!isNcpDisabled);

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    set_checkBoxTlsAuthChecked(isTlsAuth);

    QString additionalClientConfig = openvpnConfig.value(config_key::additional_client_config).
            toString(protocols::openvpn::defaultAdditionalClientConfig);
    set_textAreaAdditionalClientConfig(additionalClientConfig);

    QString additionalServerConfig = openvpnConfig.value(config_key::additional_server_config).
            toString(protocols::openvpn::defaultAdditionalServerConfig);
    set_textAreaAdditionalServerConfig(additionalServerConfig);

    if (container == DockerContainer::ShadowSocks) {
        set_radioButtonUdpEnabled(false);
        set_radioButtonTcpEnabled(false);
        set_radioButtonTcpChecked(true);
    }

    set_lineEditPortText(openvpnConfig.value(config_key::port).
                                    toString(protocols::openvpn::defaultPort));

    set_lineEditPortEnabled(container == DockerContainer::OpenVpn);

    auto lastConfig = openvpnConfig.value(config_key::last_config).toString();
    auto lastConfigJson = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    QStringList lines = lastConfigJson.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &l: lines) {
            m_openVpnLastConfigText.append(l + "\n");
    }

    emit openVpnLastConfigTextChanged(m_openVpnLastConfigText);
    set_isThirdPartyConfig(isThirdPartyConfig);
}

void OpenVpnLogic::onPushButtonProtoOpenVpnSaveClicked()
{
    QJsonObject protocolConfig = m_settings->protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Proto::OpenVpn);
    protocolConfig = getProtocolConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings->containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Proto::OpenVpn), protocolConfig);

    UiLogic::PageFunc page_proto_openvpn;
    page_proto_openvpn.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_openvpn_save;
    pushButton_proto_openvpn_save.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_openvpn_info;
    label_proto_openvpn_info.setVisibleFunc = [this] (bool visible) ->void {
        set_labelProtoOpenVpnInfoVisible(visible);
    };
    label_proto_openvpn_info.setTextFunc = [this] (const QString& text) ->void {
        set_labelProtoOpenVpnInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_openvpn_reset;
    progressBar_proto_openvpn_reset.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBarResetVisible(visible);
    };
    progressBar_proto_openvpn_reset.setValueFunc = [this] (int value) ->void {
        set_progressBarResetValue(value);
    };
    progressBar_proto_openvpn_reset.getValueFunc = [this] (void) -> int {
        return progressBarResetValue();
    };
    progressBar_proto_openvpn_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBarResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, &newContainerConfig](){
        return m_serverController->updateContainer(m_settings->serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_openvpn, progressBar_proto_openvpn_reset,
    pushButton_proto_openvpn_save, label_proto_openvpn_info);

    if (!e) {
        m_settings->setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings->clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}

QJsonObject OpenVpnLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::subnet_address, lineEditSubnetText());
    oldConfig.insert(config_key::transport_proto,
        ProtocolProps::transportProtoToString(radioButtonUdpChecked() ? ProtocolEnumNS::Udp : ProtocolEnumNS::Tcp));

    oldConfig.insert(config_key::ncp_disable, ! checkBoxAutoEncryptionChecked());
    oldConfig.insert(config_key::cipher, comboBoxVpnCipherText());
    oldConfig.insert(config_key::hash, comboBoxVpnHashText());
    oldConfig.insert(config_key::block_outside_dns, checkBoxBlockDnsChecked());
    oldConfig.insert(config_key::port, lineEditPortText());
    oldConfig.insert(config_key::tls_auth, checkBoxTlsAuthChecked());
    oldConfig.insert(config_key::additional_client_config, textAreaAdditionalClientConfig());
    oldConfig.insert(config_key::additional_server_config, textAreaAdditionalServerConfig());
    return oldConfig;
}
