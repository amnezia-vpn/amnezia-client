#include "ShadowSocksLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

ShadowSocksLogic::ShadowSocksLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_widgetProtoSsEnabled{false},
    m_comboBoxProtoShadowsocksCipherText{"chacha20-poly1305"},
    m_lineEditProtoShadowsocksPortText{},
    m_pushButtonProtoShadowsocksSaveVisible{false},
    m_progressBarProtoShadowsocksResetVisible{false},
    m_lineEditProtoShadowsocksPortEnabled{false},
    m_pageProtoShadowsocksEnabled{true},
    m_labelProtoShadowsocksInfoVisible{true},
    m_labelProtoShadowsocksInfoText{},
    m_progressBarProtoShadowsocksResetValue{0},
    m_progressBarProtoShadowsocksResetMaximium{100}
{

}

void ShadowSocksLogic::updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData)
{
    setWidgetProtoSsEnabled(haveAuthData);
    setPushButtonProtoShadowsocksSaveVisible(haveAuthData);
    setProgressBarProtoShadowsocksResetVisible(haveAuthData);

    setComboBoxProtoShadowsocksCipherText(ssConfig.value(config_key::cipher).
                                          toString(protocols::shadowsocks::defaultCipher));

    setLineEditProtoShadowsocksPortText(ssConfig.value(config_key::port).
                                        toString(protocols::shadowsocks::defaultPort));

    setLineEditProtoShadowsocksPortEnabled(container == DockerContainer::OpenVpnOverShadowSocks);
}

QJsonObject ShadowSocksLogic::getShadowSocksConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, getComboBoxProtoShadowsocksCipherText());
    oldConfig.insert(config_key::port, getLineEditProtoShadowsocksPortText());

    return oldConfig;
}

QString ShadowSocksLogic::getComboBoxProtoShadowsocksCipherText() const
{
    return m_comboBoxProtoShadowsocksCipherText;
}

void ShadowSocksLogic::setComboBoxProtoShadowsocksCipherText(const QString &comboBoxProtoShadowsocksCipherText)
{
    if (m_comboBoxProtoShadowsocksCipherText != comboBoxProtoShadowsocksCipherText) {
        m_comboBoxProtoShadowsocksCipherText = comboBoxProtoShadowsocksCipherText;
        emit comboBoxProtoShadowsocksCipherTextChanged();
    }
}

QString ShadowSocksLogic::getLineEditProtoShadowsocksPortText() const
{
    return m_lineEditProtoShadowsocksPortText;
}

void ShadowSocksLogic::setLineEditProtoShadowsocksPortText(const QString &lineEditProtoShadowsocksPortText)
{
    if (m_lineEditProtoShadowsocksPortText != lineEditProtoShadowsocksPortText) {
        m_lineEditProtoShadowsocksPortText = lineEditProtoShadowsocksPortText;
        emit lineEditProtoShadowsocksPortTextChanged();
    }
}

bool ShadowSocksLogic::getPushButtonProtoShadowsocksSaveVisible() const
{
    return m_pushButtonProtoShadowsocksSaveVisible;
}

void ShadowSocksLogic::setPushButtonProtoShadowsocksSaveVisible(bool pushButtonProtoShadowsocksSaveVisible)
{
    if (m_pushButtonProtoShadowsocksSaveVisible != pushButtonProtoShadowsocksSaveVisible) {
        m_pushButtonProtoShadowsocksSaveVisible = pushButtonProtoShadowsocksSaveVisible;
        emit pushButtonProtoShadowsocksSaveVisibleChanged();
    }
}

bool ShadowSocksLogic::getProgressBarProtoShadowsocksResetVisible() const
{
    return m_progressBarProtoShadowsocksResetVisible;
}

void ShadowSocksLogic::setProgressBarProtoShadowsocksResetVisible(bool progressBarProtoShadowsocksResetVisible)
{
    if (m_progressBarProtoShadowsocksResetVisible != progressBarProtoShadowsocksResetVisible) {
        m_progressBarProtoShadowsocksResetVisible = progressBarProtoShadowsocksResetVisible;
        emit progressBarProtoShadowsocksResetVisibleChanged();
    }
}

bool ShadowSocksLogic::getLineEditProtoShadowsocksPortEnabled() const
{
    return m_lineEditProtoShadowsocksPortEnabled;
}

void ShadowSocksLogic::setLineEditProtoShadowsocksPortEnabled(bool lineEditProtoShadowsocksPortEnabled)
{
    if (m_lineEditProtoShadowsocksPortEnabled != lineEditProtoShadowsocksPortEnabled) {
        m_lineEditProtoShadowsocksPortEnabled = lineEditProtoShadowsocksPortEnabled;
        emit lineEditProtoShadowsocksPortEnabledChanged();
    }
}

bool ShadowSocksLogic::getPageProtoShadowsocksEnabled() const
{
    return m_pageProtoShadowsocksEnabled;
}

void ShadowSocksLogic::setPageProtoShadowsocksEnabled(bool pageProtoShadowsocksEnabled)
{
    if (m_pageProtoShadowsocksEnabled != pageProtoShadowsocksEnabled) {
        m_pageProtoShadowsocksEnabled = pageProtoShadowsocksEnabled;
        emit pageProtoShadowsocksEnabledChanged();
    }
}

bool ShadowSocksLogic::getLabelProtoShadowsocksInfoVisible() const
{
    return m_labelProtoShadowsocksInfoVisible;
}

void ShadowSocksLogic::setLabelProtoShadowsocksInfoVisible(bool labelProtoShadowsocksInfoVisible)
{
    if (m_labelProtoShadowsocksInfoVisible != labelProtoShadowsocksInfoVisible) {
        m_labelProtoShadowsocksInfoVisible = labelProtoShadowsocksInfoVisible;
        emit labelProtoShadowsocksInfoVisibleChanged();
    }
}

QString ShadowSocksLogic::getLabelProtoShadowsocksInfoText() const
{
    return m_labelProtoShadowsocksInfoText;
}

void ShadowSocksLogic::setLabelProtoShadowsocksInfoText(const QString &labelProtoShadowsocksInfoText)
{
    if (m_labelProtoShadowsocksInfoText != labelProtoShadowsocksInfoText) {
        m_labelProtoShadowsocksInfoText = labelProtoShadowsocksInfoText;
        emit labelProtoShadowsocksInfoTextChanged();
    }
}

int ShadowSocksLogic::getProgressBarProtoShadowsocksResetValue() const
{
    return m_progressBarProtoShadowsocksResetValue;
}

void ShadowSocksLogic::setProgressBarProtoShadowsocksResetValue(int progressBarProtoShadowsocksResetValue)
{
    if (m_progressBarProtoShadowsocksResetValue != progressBarProtoShadowsocksResetValue) {
        m_progressBarProtoShadowsocksResetValue = progressBarProtoShadowsocksResetValue;
        emit progressBarProtoShadowsocksResetValueChanged();
    }
}

int ShadowSocksLogic::getProgressBarProtoShadowsocksResetMaximium() const
{
    return m_progressBarProtoShadowsocksResetMaximium;
}

void ShadowSocksLogic::setProgressBarProtoShadowsocksResetMaximium(int progressBarProtoShadowsocksResetMaximium)
{
    if (m_progressBarProtoShadowsocksResetMaximium != progressBarProtoShadowsocksResetMaximium) {
        m_progressBarProtoShadowsocksResetMaximium = progressBarProtoShadowsocksResetMaximium;
        emit progressBarProtoShadowsocksResetMaximiumChanged();
    }
}

bool ShadowSocksLogic::getWidgetProtoSsEnabled() const
{
    return m_widgetProtoSsEnabled;
}

void ShadowSocksLogic::setWidgetProtoSsEnabled(bool widgetProtoSsEnabled)
{
    if (m_widgetProtoSsEnabled != widgetProtoSsEnabled) {
        m_widgetProtoSsEnabled = widgetProtoSsEnabled;
        emit widgetProtoSsEnabledChanged();
    }
}

void ShadowSocksLogic::onPushButtonProtoShadowsocksSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(m_uiLogic->selectedServerIndex, m_uiLogic->selectedDockerContainer, Protocol::ShadowSocks);
    protocolConfig = getShadowSocksConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(m_uiLogic->selectedServerIndex, m_uiLogic->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::shadowsocks, protocolConfig);
    UiLogic::PageFunc page_proto_shadowsocks;
    page_proto_shadowsocks.setEnabledFunc = [this] (bool enabled) -> void {
        setPageProtoShadowsocksEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_shadowsocks_save;
    pushButton_proto_shadowsocks_save.setVisibleFunc = [this] (bool visible) ->void {
        setPushButtonProtoShadowsocksSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_shadowsocks_info;
    label_proto_shadowsocks_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelProtoShadowsocksInfoVisible(visible);
    };
    label_proto_shadowsocks_info.setTextFunc = [this] (const QString& text) ->void {
        setLabelProtoShadowsocksInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_shadowsocks_reset;
    progressBar_proto_shadowsocks_reset.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarProtoShadowsocksResetVisible(visible);
    };
    progressBar_proto_shadowsocks_reset.setValueFunc = [this] (int value) ->void {
        setProgressBarProtoShadowsocksResetValue(value);
    };
    progressBar_proto_shadowsocks_reset.getValueFunc = [this] (void) -> int {
        return getProgressBarProtoShadowsocksResetValue();
    };
    progressBar_proto_shadowsocks_reset.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarProtoShadowsocksResetMaximium();
    };

    ErrorCode e = m_uiLogic->doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(m_uiLogic->selectedServerIndex), m_uiLogic->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_shadowsocks, progressBar_proto_shadowsocks_reset,
    pushButton_proto_shadowsocks_save, label_proto_shadowsocks_info);

    if (!e) {
        m_settings.setContainerConfig(m_uiLogic->selectedServerIndex, m_uiLogic->selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(m_uiLogic->selectedServerIndex, m_uiLogic->selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << m_uiLogic->selectedServerIndex << m_uiLogic->selectedDockerContainer;
}
