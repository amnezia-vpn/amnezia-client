#include "ShadowSocksLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

ShadowSocksLogic::ShadowSocksLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent),
    m_comboBoxCipherText{"chacha20-poly1305"},
    m_lineEditPortText{},
    m_pushButtonSaveVisible{false},
    m_progressBaResetVisible{false},
    m_lineEditPortEnabled{false},
    m_labelInfoVisible{true},
    m_labelInfoText{},
    m_progressBaResetValue{0},
    m_progressBaResetMaximium{100}
{

}

void ShadowSocksLogic::updateProtocolPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData)
{
    set_pageEnabled(haveAuthData);
    set_pushButtonSaveVisible(haveAuthData);
    set_progressBaResetVisible(haveAuthData);

    set_comboBoxCipherText(ssConfig.value(config_key::cipher).
                                          toString(protocols::shadowsocks::defaultCipher));

    set_lineEditPortText(ssConfig.value(config_key::port).
                                        toString(protocols::shadowsocks::defaultPort));

    set_lineEditPortEnabled(container == DockerContainer::ShadowSocks);
}

QJsonObject ShadowSocksLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, comboBoxCipherText());
    oldConfig.insert(config_key::port, lineEditPortText());

    return oldConfig;
}

void ShadowSocksLogic::onPushButtonSaveClicked()
{
    QJsonObject protocolConfig = m_settings->protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Proto::ShadowSocks);

    QJsonObject containerConfig = m_settings->containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Proto::ShadowSocks), protocolConfig);
    UiLogic::PageFunc page_proto_shadowsocks;
    page_proto_shadowsocks.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_shadowsocks_save;
    pushButton_proto_shadowsocks_save.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_shadowsocks_info;
    label_proto_shadowsocks_info.setVisibleFunc = [this] (bool visible) ->void {
        set_labelInfoVisible(visible);
    };
    label_proto_shadowsocks_info.setTextFunc = [this] (const QString& text) ->void {
        set_labelInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_reset;
    progressBar_reset.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBaResetVisible(visible);
    };
    progressBar_reset.setValueFunc = [this] (int value) ->void {
        set_progressBaResetValue(value);
    };
    progressBar_reset.getValueFunc = [this] (void) -> int {
        return progressBaResetValue();
    };
    progressBar_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBaResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, &newContainerConfig](){
        return m_serverController->updateContainer(m_settings->serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_shadowsocks, progressBar_reset,
    pushButton_proto_shadowsocks_save, label_proto_shadowsocks_info);

    if (!e) {
        m_settings->setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings->clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}
