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
    m_pushButtonProtoOpenvpnSaveVisible{false},
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
    setWidgetProtoOpenvpnEnabled(haveAuthData);
    setPushButtonProtoOpenvpnSaveVisible(haveAuthData);
    setProgressBarProtoOpenvpnResetVisible(haveAuthData);

    setRadioButtonProtoOpenvpnUdpEnabled(true);
    setRadioButtonProtoOpenvpnTcpEnabled(true);

    setLineEditProtoOpenvpnSubnetText(openvpnConfig.value(config_key::subnet_address).
                                      toString(protocols::openvpn::defaultSubnetAddress));

    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
            toString(protocols::openvpn::defaultTransportProto);

    setRadioButtonProtoOpenvpnUdpChecked(trasnsport == protocols::openvpn::defaultTransportProto);
    setRadioButtonProtoOpenvpnTcpChecked(trasnsport != protocols::openvpn::defaultTransportProto);

    setComboBoxProtoOpenvpnCipherText(openvpnConfig.value(config_key::cipher).
                                      toString(protocols::openvpn::defaultCipher));

    setComboBoxProtoOpenvpnHashText(openvpnConfig.value(config_key::hash).
                                    toString(protocols::openvpn::defaultHash));

    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    setCheckBoxProtoOpenvpnBlockDnsChecked(blockOutsideDns);

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    setCheckBoxProtoOpenvpnAutoEncryptionChecked(!isNcpDisabled);

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    setCheckBoxProtoOpenvpnTlsAuthChecked(isTlsAuth);

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        setRadioButtonProtoOpenvpnUdpEnabled(false);
        setRadioButtonProtoOpenvpnTcpEnabled(false);
        setRadioButtonProtoOpenvpnTcpChecked(true);
    }

    setLineEditProtoOpenvpnPortText(openvpnConfig.value(config_key::port).
                                    toString(protocols::openvpn::defaultPort));

    setLineEditProtoOpenvpnPortEnabled(container == DockerContainer::OpenVpn);
}

QString OpenVpnLogic::getLineEditProtoOpenvpnSubnetText() const
{
    return m_lineEditProtoOpenvpnSubnetText;
}

void OpenVpnLogic::setLineEditProtoOpenvpnSubnetText(const QString &lineEditProtoOpenvpnSubnetText)
{
    if (m_lineEditProtoOpenvpnSubnetText != lineEditProtoOpenvpnSubnetText) {
        m_lineEditProtoOpenvpnSubnetText = lineEditProtoOpenvpnSubnetText;
        emit lineEditProtoOpenvpnSubnetTextChanged();
    }
}

bool OpenVpnLogic::getRadioButtonProtoOpenvpnUdpChecked() const
{
    return m_radioButtonProtoOpenvpnUdpChecked;
}

void OpenVpnLogic::setRadioButtonProtoOpenvpnUdpChecked(bool radioButtonProtoOpenvpnUdpChecked)
{
    if (m_radioButtonProtoOpenvpnUdpChecked != radioButtonProtoOpenvpnUdpChecked) {
        m_radioButtonProtoOpenvpnUdpChecked = radioButtonProtoOpenvpnUdpChecked;
        emit radioButtonProtoOpenvpnUdpCheckedChanged();
    }
}

bool OpenVpnLogic::getCheckBoxProtoOpenvpnAutoEncryptionChecked() const
{
    return m_checkBoxProtoOpenvpnAutoEncryptionChecked;
}

void OpenVpnLogic::setCheckBoxProtoOpenvpnAutoEncryptionChecked(bool checkBoxProtoOpenvpnAutoEncryptionChecked)
{
    if (m_checkBoxProtoOpenvpnAutoEncryptionChecked != checkBoxProtoOpenvpnAutoEncryptionChecked) {
        m_checkBoxProtoOpenvpnAutoEncryptionChecked = checkBoxProtoOpenvpnAutoEncryptionChecked;
        emit checkBoxProtoOpenvpnAutoEncryptionCheckedChanged();
    }
}

QString OpenVpnLogic::getComboBoxProtoOpenvpnCipherText() const
{
    return m_comboBoxProtoOpenvpnCipherText;
}

void OpenVpnLogic::setComboBoxProtoOpenvpnCipherText(const QString &comboBoxProtoOpenvpnCipherText)
{
    if (m_comboBoxProtoOpenvpnCipherText != comboBoxProtoOpenvpnCipherText) {
        m_comboBoxProtoOpenvpnCipherText = comboBoxProtoOpenvpnCipherText;
        emit comboBoxProtoOpenvpnCipherTextChanged();
    }
}

QString OpenVpnLogic::getComboBoxProtoOpenvpnHashText() const
{
    return m_comboBoxProtoOpenvpnHashText;
}

void OpenVpnLogic::setComboBoxProtoOpenvpnHashText(const QString &comboBoxProtoOpenvpnHashText)
{
    if (m_comboBoxProtoOpenvpnHashText != comboBoxProtoOpenvpnHashText) {
        m_comboBoxProtoOpenvpnHashText = comboBoxProtoOpenvpnHashText;
        emit comboBoxProtoOpenvpnHashTextChanged();
    }
}

bool OpenVpnLogic::getCheckBoxProtoOpenvpnBlockDnsChecked() const
{
    return m_checkBoxProtoOpenvpnBlockDnsChecked;
}

void OpenVpnLogic::setCheckBoxProtoOpenvpnBlockDnsChecked(bool checkBoxProtoOpenvpnBlockDnsChecked)
{
    if (m_checkBoxProtoOpenvpnBlockDnsChecked != checkBoxProtoOpenvpnBlockDnsChecked) {
        m_checkBoxProtoOpenvpnBlockDnsChecked = checkBoxProtoOpenvpnBlockDnsChecked;
        emit checkBoxProtoOpenvpnBlockDnsCheckedChanged();
    }
}

QString OpenVpnLogic::getLineEditProtoOpenvpnPortText() const
{
    return m_lineEditProtoOpenvpnPortText;
}

void OpenVpnLogic::setLineEditProtoOpenvpnPortText(const QString &lineEditProtoOpenvpnPortText)
{
    if (m_lineEditProtoOpenvpnPortText != lineEditProtoOpenvpnPortText) {
        m_lineEditProtoOpenvpnPortText = lineEditProtoOpenvpnPortText;
        emit lineEditProtoOpenvpnPortTextChanged();
    }
}

bool OpenVpnLogic::getCheckBoxProtoOpenvpnTlsAuthChecked() const
{
    return m_checkBoxProtoOpenvpnTlsAuthChecked;
}

void OpenVpnLogic::setCheckBoxProtoOpenvpnTlsAuthChecked(bool checkBoxProtoOpenvpnTlsAuthChecked)
{
    if (m_checkBoxProtoOpenvpnTlsAuthChecked != checkBoxProtoOpenvpnTlsAuthChecked) {
        m_checkBoxProtoOpenvpnTlsAuthChecked = checkBoxProtoOpenvpnTlsAuthChecked;
        emit checkBoxProtoOpenvpnTlsAuthCheckedChanged();
    }
}

bool OpenVpnLogic::getWidgetProtoOpenvpnEnabled() const
{
    return m_widgetProtoOpenvpnEnabled;
}

void OpenVpnLogic::setWidgetProtoOpenvpnEnabled(bool widgetProtoOpenvpnEnabled)
{
    if (m_widgetProtoOpenvpnEnabled != widgetProtoOpenvpnEnabled) {
        m_widgetProtoOpenvpnEnabled = widgetProtoOpenvpnEnabled;
        emit widgetProtoOpenvpnEnabledChanged();
    }
}

bool OpenVpnLogic::getPushButtonProtoOpenvpnSaveVisible() const
{
    return m_pushButtonProtoOpenvpnSaveVisible;
}

void OpenVpnLogic::setPushButtonProtoOpenvpnSaveVisible(bool pushButtonProtoOpenvpnSaveVisible)
{
    if (m_pushButtonProtoOpenvpnSaveVisible != pushButtonProtoOpenvpnSaveVisible) {
        m_pushButtonProtoOpenvpnSaveVisible = pushButtonProtoOpenvpnSaveVisible;
        emit pushButtonProtoOpenvpnSaveVisibleChanged();
    }
}

bool OpenVpnLogic::getProgressBarProtoOpenvpnResetVisible() const
{
    return m_progressBarProtoOpenvpnResetVisible;
}

void OpenVpnLogic::setProgressBarProtoOpenvpnResetVisible(bool progressBarProtoOpenvpnResetVisible)
{
    if (m_progressBarProtoOpenvpnResetVisible != progressBarProtoOpenvpnResetVisible) {
        m_progressBarProtoOpenvpnResetVisible = progressBarProtoOpenvpnResetVisible;
        emit progressBarProtoOpenvpnResetVisibleChanged();
    }
}

bool OpenVpnLogic::getRadioButtonProtoOpenvpnUdpEnabled() const
{
    return m_radioButtonProtoOpenvpnUdpEnabled;
}



void OpenVpnLogic::setRadioButtonProtoOpenvpnUdpEnabled(bool radioButtonProtoOpenvpnUdpEnabled)
{
    if (m_radioButtonProtoOpenvpnUdpEnabled != radioButtonProtoOpenvpnUdpEnabled) {
        m_radioButtonProtoOpenvpnUdpEnabled = radioButtonProtoOpenvpnUdpEnabled;
        emit radioButtonProtoOpenvpnUdpEnabledChanged();
    }
}

bool OpenVpnLogic::getRadioButtonProtoOpenvpnTcpEnabled() const
{
    return m_radioButtonProtoOpenvpnTcpEnabled;
}

void OpenVpnLogic::setRadioButtonProtoOpenvpnTcpEnabled(bool radioButtonProtoOpenvpnTcpEnabled)
{
    if (m_radioButtonProtoOpenvpnTcpEnabled != radioButtonProtoOpenvpnTcpEnabled) {
        m_radioButtonProtoOpenvpnTcpEnabled = radioButtonProtoOpenvpnTcpEnabled;
        emit radioButtonProtoOpenvpnTcpEnabledChanged();
    }
}

bool OpenVpnLogic::getRadioButtonProtoOpenvpnTcpChecked() const
{
    return m_radioButtonProtoOpenvpnTcpChecked;
}

void OpenVpnLogic::setRadioButtonProtoOpenvpnTcpChecked(bool radioButtonProtoOpenvpnTcpChecked)
{
    if (m_radioButtonProtoOpenvpnTcpChecked != radioButtonProtoOpenvpnTcpChecked) {
        m_radioButtonProtoOpenvpnTcpChecked = radioButtonProtoOpenvpnTcpChecked;
        emit radioButtonProtoOpenvpnTcpCheckedChanged();
    }
}

bool OpenVpnLogic::getLineEditProtoOpenvpnPortEnabled() const
{
    return m_lineEditProtoOpenvpnPortEnabled;
}

void OpenVpnLogic::setLineEditProtoOpenvpnPortEnabled(bool lineEditProtoOpenvpnPortEnabled)
{
    if (m_lineEditProtoOpenvpnPortEnabled != lineEditProtoOpenvpnPortEnabled) {
        m_lineEditProtoOpenvpnPortEnabled = lineEditProtoOpenvpnPortEnabled;
        emit lineEditProtoOpenvpnPortEnabledChanged();
    }
}

bool OpenVpnLogic::getComboBoxProtoOpenvpnCipherEnabled() const
{
    return m_comboBoxProtoOpenvpnCipherEnabled;
}

void OpenVpnLogic::setComboBoxProtoOpenvpnCipherEnabled(bool comboBoxProtoOpenvpnCipherEnabled)
{
    if (m_comboBoxProtoOpenvpnCipherEnabled != comboBoxProtoOpenvpnCipherEnabled) {
        m_comboBoxProtoOpenvpnCipherEnabled = comboBoxProtoOpenvpnCipherEnabled;
        emit comboBoxProtoOpenvpnCipherEnabledChanged();
    }
}

bool OpenVpnLogic::getComboBoxProtoOpenvpnHashEnabled() const
{
    return m_comboBoxProtoOpenvpnHashEnabled;
}

void OpenVpnLogic::setComboBoxProtoOpenvpnHashEnabled(bool comboBoxProtoOpenvpnHashEnabled)
{
    if (m_comboBoxProtoOpenvpnHashEnabled != comboBoxProtoOpenvpnHashEnabled) {
        m_comboBoxProtoOpenvpnHashEnabled = comboBoxProtoOpenvpnHashEnabled;
        emit comboBoxProtoOpenvpnHashEnabledChanged();
    }
}
bool OpenVpnLogic::getPageProtoOpenvpnEnabled() const
{
    return m_pageProtoOpenvpnEnabled;
}

void OpenVpnLogic::setPageProtoOpenvpnEnabled(bool pageProtoOpenvpnEnabled)
{
    if (m_pageProtoOpenvpnEnabled != pageProtoOpenvpnEnabled) {
        m_pageProtoOpenvpnEnabled = pageProtoOpenvpnEnabled;
        emit pageProtoOpenvpnEnabledChanged();
    }
}

bool OpenVpnLogic::getLabelProtoOpenvpnInfoVisible() const
{
    return m_labelProtoOpenvpnInfoVisible;
}

void OpenVpnLogic::setLabelProtoOpenvpnInfoVisible(bool labelProtoOpenvpnInfoVisible)
{
    if (m_labelProtoOpenvpnInfoVisible != labelProtoOpenvpnInfoVisible) {
        m_labelProtoOpenvpnInfoVisible = labelProtoOpenvpnInfoVisible;
        emit labelProtoOpenvpnInfoVisibleChanged();
    }
}

QString OpenVpnLogic::getLabelProtoOpenvpnInfoText() const
{
    return m_labelProtoOpenvpnInfoText;
}

void OpenVpnLogic::setLabelProtoOpenvpnInfoText(const QString &labelProtoOpenvpnInfoText)
{
    if (m_labelProtoOpenvpnInfoText != labelProtoOpenvpnInfoText) {
        m_labelProtoOpenvpnInfoText = labelProtoOpenvpnInfoText;
        emit labelProtoOpenvpnInfoTextChanged();
    }
}

int OpenVpnLogic::getProgressBarProtoOpenvpnResetValue() const
{
    return m_progressBarProtoOpenvpnResetValue;
}

void OpenVpnLogic::setProgressBarProtoOpenvpnResetValue(int progressBarProtoOpenvpnResetValue)
{
    if (m_progressBarProtoOpenvpnResetValue != progressBarProtoOpenvpnResetValue) {
        m_progressBarProtoOpenvpnResetValue = progressBarProtoOpenvpnResetValue;
        emit progressBarProtoOpenvpnResetValueChanged();
    }
}

int OpenVpnLogic::getProgressBarProtoOpenvpnResetMaximium() const
{
    return m_progressBarProtoOpenvpnResetMaximium;
}

void OpenVpnLogic::setProgressBarProtoOpenvpnResetMaximium(int progressBarProtoOpenvpnResetMaximium)
{
    if (m_progressBarProtoOpenvpnResetMaximium != progressBarProtoOpenvpnResetMaximium) {
        m_progressBarProtoOpenvpnResetMaximium = progressBarProtoOpenvpnResetMaximium;
        emit progressBarProtoOpenvpnResetMaximiumChanged();
    }
}

void OpenVpnLogic::onCheckBoxProtoOpenvpnAutoEncryptionClicked()
{
    setComboBoxProtoOpenvpnCipherEnabled(!getCheckBoxProtoOpenvpnAutoEncryptionChecked());
    setComboBoxProtoOpenvpnHashEnabled(!getCheckBoxProtoOpenvpnAutoEncryptionChecked());
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
        setPageProtoOpenvpnEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_openvpn_save;
    pushButton_proto_openvpn_save.setVisibleFunc = [this] (bool visible) ->void {
        setPushButtonProtoOpenvpnSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_openvpn_info;
    label_proto_openvpn_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelProtoOpenvpnInfoVisible(visible);
    };
    label_proto_openvpn_info.setTextFunc = [this] (const QString& text) ->void {
        setLabelProtoOpenvpnInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_openvpn_reset;
    progressBar_proto_openvpn_reset.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarProtoOpenvpnResetVisible(visible);
    };
    progressBar_proto_openvpn_reset.setValueFunc = [this] (int value) ->void {
        setProgressBarProtoOpenvpnResetValue(value);
    };
    progressBar_proto_openvpn_reset.getValueFunc = [this] (void) -> int {
        return getProgressBarProtoOpenvpnResetValue();
    };
    progressBar_proto_openvpn_reset.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarProtoOpenvpnResetMaximium();
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
    oldConfig.insert(config_key::subnet_address, getLineEditProtoOpenvpnSubnetText());
    oldConfig.insert(config_key::transport_proto, getRadioButtonProtoOpenvpnUdpChecked() ? protocols::UDP : protocols::TCP);
    oldConfig.insert(config_key::ncp_disable, ! getCheckBoxProtoOpenvpnAutoEncryptionChecked());
    oldConfig.insert(config_key::cipher, getComboBoxProtoOpenvpnCipherText());
    oldConfig.insert(config_key::hash, getComboBoxProtoOpenvpnHashText());
    oldConfig.insert(config_key::block_outside_dns, getCheckBoxProtoOpenvpnBlockDnsChecked());
    oldConfig.insert(config_key::port, getLineEditProtoOpenvpnPortText());
    oldConfig.insert(config_key::tls_auth, getCheckBoxProtoOpenvpnTlsAuthChecked());
    return oldConfig;
}
