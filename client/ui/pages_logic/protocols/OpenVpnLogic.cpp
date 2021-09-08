#include "OpenVpnLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

OpenVpnLogic::OpenVpnLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_lineEditProtoOpenvpnSubnetText{},
    m_radioButtonProtoOpenvpnUdpChecked{false},
    m_checkBoxProtoOpenvpnAutoEncryptionChecked{},
    m_comboBoxProtoOpenvpnCipherText{"AES-256-GCM"},
    m_comboBoxProtoOpenvpnHashText{"SHA512"},
    m_checkBoxProtoOpenvpnBlockDnsChecked{false},
    m_lineEditProtoOpenvpnPortText{},
    m_checkBoxProtoOpenvpnTlsAuthChecked{false},
    m_widgetProtoOpenvpnEnabled{false},
    m_pushButtonOpenvpnSaveVisible{false},
    m_progressBarProtoOpenvpnResetVisible{false},
    m_radioButtonProtoOpenvpnUdpEnabled{false},
    m_radioButtonProtoOpenvpnTcpEnabled{false},
    m_radioButtonProtoOpenvpnTcpChecked{false},
    m_lineEditProtoOpenvpnPortEnabled{false},
    m_comboBoxProtoOpenvpnCipherEnabled{true},
    m_comboBoxProtoOpenvpnHashEnabled{true},
    m_pageProtoOpenvpnEnabled{true},
    m_labelProtoOpenvpnInfoVisible{true},
    m_labelProtoOpenvpnInfoText{},
    m_progressBarProtoOpenvpnResetValue{0},
    m_progressBarProtoOpenvpnResetMaximium{100}
{

}

void OpenVpnLogic::updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData)
{
    set_widgetProtoOpenvpnEnabled(haveAuthData);
    set_pushButtonOpenvpnSaveVisible(haveAuthData);
    set_progressBarProtoOpenvpnResetVisible(haveAuthData);

    set_radioButtonProtoOpenvpnUdpEnabled(true);
    set_radioButtonProtoOpenvpnTcpEnabled(true);

    set_lineEditProtoOpenvpnSubnetText(openvpnConfig.value(config_key::subnet_address).
                                      toString(protocols::openvpn::defaultSubnetAddress));

    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
            toString(protocols::openvpn::defaultTransportProto);

    set_radioButtonProtoOpenvpnUdpChecked(trasnsport == protocols::openvpn::defaultTransportProto);
    set_radioButtonProtoOpenvpnTcpChecked(trasnsport != protocols::openvpn::defaultTransportProto);

    set_comboBoxProtoOpenvpnCipherText(openvpnConfig.value(config_key::cipher).
                                      toString(protocols::openvpn::defaultCipher));

    set_comboBoxProtoOpenvpnHashText(openvpnConfig.value(config_key::hash).
                                    toString(protocols::openvpn::defaultHash));

    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    set_checkBoxProtoOpenvpnBlockDnsChecked(blockOutsideDns);

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    set_checkBoxProtoOpenvpnAutoEncryptionChecked(!isNcpDisabled);

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    set_checkBoxProtoOpenvpnTlsAuthChecked(isTlsAuth);

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        set_radioButtonProtoOpenvpnUdpEnabled(false);
        set_radioButtonProtoOpenvpnTcpEnabled(false);
        set_radioButtonProtoOpenvpnTcpChecked(true);
    }

    set_lineEditProtoOpenvpnPortText(openvpnConfig.value(config_key::port).
                                    toString(protocols::openvpn::defaultPort));

    set_lineEditProtoOpenvpnPortEnabled(container == DockerContainer::OpenVpn);
}

void OpenVpnLogic::onCheckBoxProtoOpenvpnAutoEncryptionClicked()
{
    set_comboBoxProtoOpenvpnCipherEnabled(!checkBoxProtoOpenvpnAutoEncryptionChecked());
    set_comboBoxProtoOpenvpnHashEnabled(!checkBoxProtoOpenvpnAutoEncryptionChecked());
}

void OpenVpnLogic::onPushButtonProtoOpenvpnSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::OpenVpn);
    protocolConfig = getOpenVpnConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::openvpn, protocolConfig);

    UiLogic::PageFunc page_proto_openvpn;
    page_proto_openvpn.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageProtoOpenvpnEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_openvpn_save;
    pushButton_proto_openvpn_save.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonOpenvpnSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_openvpn_info;
    label_proto_openvpn_info.setVisibleFunc = [this] (bool visible) ->void {
        set_labelProtoOpenvpnInfoVisible(visible);
    };
    label_proto_openvpn_info.setTextFunc = [this] (const QString& text) ->void {
        set_labelProtoOpenvpnInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_openvpn_reset;
    progressBar_proto_openvpn_reset.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBarProtoOpenvpnResetVisible(visible);
    };
    progressBar_proto_openvpn_reset.setValueFunc = [this] (int value) ->void {
        set_progressBarProtoOpenvpnResetValue(value);
    };
    progressBar_proto_openvpn_reset.getValueFunc = [this] (void) -> int {
        return progressBarProtoOpenvpnResetValue();
    };
    progressBar_proto_openvpn_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBarProtoOpenvpnResetMaximium();
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

QJsonObject OpenVpnLogic::getOpenVpnConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::subnet_address, lineEditProtoOpenvpnSubnetText());
    oldConfig.insert(config_key::transport_proto, radioButtonProtoOpenvpnUdpChecked() ? protocols::UDP : protocols::TCP);
    oldConfig.insert(config_key::ncp_disable, ! checkBoxProtoOpenvpnAutoEncryptionChecked());
    oldConfig.insert(config_key::cipher, comboBoxProtoOpenvpnCipherText());
    oldConfig.insert(config_key::hash, comboBoxProtoOpenvpnHashText());
    oldConfig.insert(config_key::block_outside_dns, checkBoxProtoOpenvpnBlockDnsChecked());
    oldConfig.insert(config_key::port, lineEditProtoOpenvpnPortText());
    oldConfig.insert(config_key::tls_auth, checkBoxProtoOpenvpnTlsAuthChecked());
    return oldConfig;
}
