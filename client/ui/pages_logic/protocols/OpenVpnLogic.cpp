#include "OpenVpnLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

OpenVpnLogic::OpenVpnLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent),
    m_lineEditProtoOpenVpnSubnetText{""},

    m_radioButtonProtoOpenVpnTcpEnabled{true},
    m_radioButtonProtoOpenVpnTcpChecked{false},
    m_radioButtonProtoOpenVpnUdpEnabled{true},
    m_radioButtonProtoOpenVpnUdpChecked{false},

    m_checkBoxProtoOpenVpnAutoEncryptionChecked{},
    m_comboBoxProtoOpenVpnCipherText{"AES-256-GCM"},
    m_comboBoxProtoOpenVpnHashText{"SHA512"},
    m_checkBoxProtoOpenVpnBlockDnsChecked{false},
    m_lineEditProtoOpenVpnPortText{},
    m_checkBoxProtoOpenVpnTlsAuthChecked{false},
    m_pushButtonOpenvpnSaveVisible{false},
    m_progressBarProtoOpenVpnResetVisible{false},

    m_lineEditProtoOpenVpnPortEnabled{false},
    m_comboBoxProtoOpenVpnCipherEnabled{true},
    m_comboBoxProtoOpenVpnHashEnabled{true},
    m_labelProtoOpenVpnInfoVisible{true},
    m_labelProtoOpenVpnInfoText{},
    m_progressBarProtoOpenVpnResetValue{0},
    m_progressBarProtoOpenVpnResetMaximium{100}
{

}

void OpenVpnLogic::updateProtocolPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData)
{
    qDebug() << "OpenVpnLogic::updateProtocolPage";
    set_pageEnabled(haveAuthData);
    set_pushButtonOpenvpnSaveVisible(haveAuthData);
    set_progressBarProtoOpenVpnResetVisible(haveAuthData);

    set_radioButtonProtoOpenVpnUdpEnabled(true);
    set_radioButtonProtoOpenVpnTcpEnabled(true);

    set_lineEditProtoOpenVpnSubnetText(openvpnConfig.value(config_key::subnet_address).
                                      toString(protocols::openvpn::defaultSubnetAddress));

    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
            toString(protocols::openvpn::defaultTransportProto);

    set_radioButtonProtoOpenVpnUdpChecked(trasnsport == protocols::openvpn::defaultTransportProto);
    set_radioButtonProtoOpenVpnTcpChecked(trasnsport != protocols::openvpn::defaultTransportProto);

    set_comboBoxProtoOpenVpnCipherText(openvpnConfig.value(config_key::cipher).
                                      toString(protocols::openvpn::defaultCipher));

    set_comboBoxProtoOpenVpnHashText(openvpnConfig.value(config_key::hash).
                                    toString(protocols::openvpn::defaultHash));

    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    set_checkBoxProtoOpenVpnBlockDnsChecked(blockOutsideDns);

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    set_checkBoxProtoOpenVpnAutoEncryptionChecked(!isNcpDisabled);

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    set_checkBoxProtoOpenVpnTlsAuthChecked(isTlsAuth);

    if (container == DockerContainer::ShadowSocks) {
        set_radioButtonProtoOpenVpnUdpEnabled(false);
        set_radioButtonProtoOpenVpnTcpEnabled(false);
        set_radioButtonProtoOpenVpnTcpChecked(true);
    }

    set_lineEditProtoOpenVpnPortText(openvpnConfig.value(config_key::port).
                                    toString(protocols::openvpn::defaultPort));

    set_lineEditProtoOpenVpnPortEnabled(container == DockerContainer::OpenVpn);
}

void OpenVpnLogic::onCheckBoxProtoOpenVpnAutoEncryptionClicked()
{
    set_comboBoxProtoOpenVpnCipherEnabled(!checkBoxProtoOpenVpnAutoEncryptionChecked());
    set_comboBoxProtoOpenVpnHashEnabled(!checkBoxProtoOpenVpnAutoEncryptionChecked());
}

void OpenVpnLogic::onPushButtonProtoOpenVpnSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::OpenVpn);
    protocolConfig = getProtocolConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Protocol::OpenVpn), protocolConfig);

    UiLogic::PageFunc page_proto_openvpn;
    page_proto_openvpn.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_openvpn_save;
    pushButton_proto_openvpn_save.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonOpenvpnSaveVisible(visible);
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
        set_progressBarProtoOpenVpnResetVisible(visible);
    };
    progressBar_proto_openvpn_reset.setValueFunc = [this] (int value) ->void {
        set_progressBarProtoOpenVpnResetValue(value);
    };
    progressBar_proto_openvpn_reset.getValueFunc = [this] (void) -> int {
        return progressBarProtoOpenVpnResetValue();
    };
    progressBar_proto_openvpn_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBarProtoOpenVpnResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_openvpn, progressBar_proto_openvpn_reset,
    pushButton_proto_openvpn_save, label_proto_openvpn_info);

    if (!e) {
        m_settings.setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}

QJsonObject OpenVpnLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::subnet_address, lineEditProtoOpenVpnSubnetText());
    oldConfig.insert(config_key::transport_proto, radioButtonProtoOpenVpnUdpChecked() ? protocols::UDP : protocols::TCP);
    oldConfig.insert(config_key::ncp_disable, ! checkBoxProtoOpenVpnAutoEncryptionChecked());
    oldConfig.insert(config_key::cipher, comboBoxProtoOpenVpnCipherText());
    oldConfig.insert(config_key::hash, comboBoxProtoOpenVpnHashText());
    oldConfig.insert(config_key::block_outside_dns, checkBoxProtoOpenVpnBlockDnsChecked());
    oldConfig.insert(config_key::port, lineEditProtoOpenVpnPortText());
    oldConfig.insert(config_key::tls_auth, checkBoxProtoOpenVpnTlsAuthChecked());
    return oldConfig;
}
