#include "CloakLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

CloakLogic::CloakLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent),
    m_comboBoxProtoCloakCipherText{"chacha20-poly1305"},
    m_lineEditProtoCloakSiteText{"tile.openstreetmap.org"},
    m_lineEditProtoCloakPortText{},
    m_widgetProtoCloakEnabled{false},
    m_pushButtonCloakSaveVisible{false},
    m_progressBarProtoCloakResetVisible{false},
    m_lineEditProtoCloakPortEnabled{false},
    m_pageProtoCloakEnabled{true},
    m_labelProtoCloakInfoVisible{true},
    m_labelProtoCloakInfoText{},
    m_progressBarProtoCloakResetValue{0},
    m_progressBarProtoCloakResetMaximium{100}
{

}

void CloakLogic::updateProtocolPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData)
{
    set_widgetProtoCloakEnabled(haveAuthData);
    set_pushButtonCloakSaveVisible(haveAuthData);
    set_progressBarProtoCloakResetVisible(haveAuthData);

    set_comboBoxProtoCloakCipherText(ckConfig.value(config_key::cipher).
                                    toString(protocols::cloak::defaultCipher));

    set_lineEditProtoCloakSiteText(ckConfig.value(config_key::site).
                                  toString(protocols::cloak::defaultRedirSite));

    set_lineEditProtoCloakPortText(ckConfig.value(config_key::port).
                                  toString(protocols::cloak::defaultPort));

    set_lineEditProtoCloakPortEnabled(container == DockerContainer::OpenVpnOverCloak);
}

QJsonObject CloakLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, comboBoxProtoCloakCipherText());
    oldConfig.insert(config_key::site, lineEditProtoCloakSiteText());
    oldConfig.insert(config_key::port, lineEditProtoCloakPortText());

    return oldConfig;
}

void CloakLogic::onPushButtonProtoCloakSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::Cloak);
    protocolConfig = getProtocolConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::cloak, protocolConfig);

    UiLogic::PageFunc page_proto_cloak;
    page_proto_cloak.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageProtoCloakEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_proto_cloak_save;
    pushButton_proto_cloak_save.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonCloakSaveVisible(visible);
    };
    UiLogic::LabelFunc label_proto_cloak_info;
    label_proto_cloak_info.setVisibleFunc = [this] (bool visible) ->void {
        set_labelProtoCloakInfoVisible(visible);
    };
    label_proto_cloak_info.setTextFunc = [this] (const QString& text) ->void {
        set_labelProtoCloakInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_proto_cloak_reset;
    progressBar_proto_cloak_reset.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBarProtoCloakResetVisible(visible);
    };
    progressBar_proto_cloak_reset.setValueFunc = [this] (int value) ->void {
        set_progressBarProtoCloakResetValue(value);
    };
    progressBar_proto_cloak_reset.getValueFunc = [this] (void) -> int {
        return progressBarProtoCloakResetValue();
    };
    progressBar_proto_cloak_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBarProtoCloakResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_cloak, progressBar_proto_cloak_reset,
    pushButton_proto_cloak_save, label_proto_cloak_info);

    if (!e) {
        m_settings.setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }

    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}
