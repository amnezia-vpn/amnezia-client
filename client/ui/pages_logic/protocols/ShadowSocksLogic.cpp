#include "ShadowSocksLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

ShadowSocksLogic::ShadowSocksLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_widgetProtoSsEnabled{false},
    m_comboBoxProtoShadowsocksCipherText{"chacha20-poly1305"},
    m_lineEditProtoShadowsocksPortText{},
    m_pushButtonShadowsocksSaveVisible{false},
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
    set_widgetProtoSsEnabled(haveAuthData);
    set_pushButtonShadowsocksSaveVisible(haveAuthData);
    set_progressBarProtoShadowsocksResetVisible(haveAuthData);

    set_comboBoxProtoShadowsocksCipherText(ssConfig.value(config_key::cipher).
                                          toString(protocols::shadowsocks::defaultCipher));

    set_lineEditProtoShadowsocksPortText(ssConfig.value(config_key::port).
                                        toString(protocols::shadowsocks::defaultPort));

    set_lineEditProtoShadowsocksPortEnabled(container == DockerContainer::OpenVpnOverShadowSocks);
}

QJsonObject ShadowSocksLogic::getShadowSocksConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, comboBoxProtoShadowsocksCipherText());
    oldConfig.insert(config_key::port, lineEditProtoShadowsocksPortText());

    return oldConfig;
}

void ShadowSocksLogic::onPushButtonProtoShadowsocksSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::ShadowSocks);
    protocolConfig = getShadowSocksConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::shadowsocks, protocolConfig);
    UiLogic::PageFunc page_proto_shadowsocks;
    page_proto_shadowsocks.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageProtoShadowsocksEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_shadowsocks_save;
    pushButton_proto_shadowsocks_save.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonShadowsocksSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_shadowsocks_info;
    label_proto_shadowsocks_info.setVisibleFunc = [this] (bool visible) ->void {
        set_labelProtoShadowsocksInfoVisible(visible);
    };
    label_proto_shadowsocks_info.setTextFunc = [this] (const QString& text) ->void {
        set_labelProtoShadowsocksInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_shadowsocks_reset;
    progressBar_proto_shadowsocks_reset.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBarProtoShadowsocksResetVisible(visible);
    };
    progressBar_proto_shadowsocks_reset.setValueFunc = [this] (int value) ->void {
        set_progressBarProtoShadowsocksResetValue(value);
    };
    progressBar_proto_shadowsocks_reset.getValueFunc = [this] (void) -> int {
        return progressBarProtoShadowsocksResetValue();
    };
    progressBar_proto_shadowsocks_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBarProtoShadowsocksResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_shadowsocks, progressBar_proto_shadowsocks_reset,
    pushButton_proto_shadowsocks_save, label_proto_shadowsocks_info);

    if (!e) {
        m_settings.setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}
