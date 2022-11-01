#include "CloakLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

CloakLogic::CloakLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent),
    m_comboBoxCipherText{"chacha20-poly1305"},
    m_lineEditSiteText{"tile.openstreetmap.org"},
    m_lineEditPortText{},
    m_pushButtonSaveVisible{false},
    m_progressBarResetVisible{false},
    m_lineEditPortEnabled{false},
    m_pageEnabled{true},
    m_labelInfoVisible{true},
    m_labelInfoText{},
    m_progressBarResetValue{0},
    m_progressBarResetMaximium{100}
{

}

void CloakLogic::updateProtocolPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData, bool isThirdPartyConfig)
{
    set_pageEnabled(haveAuthData);
    set_pushButtonSaveVisible(haveAuthData);
    set_progressBarResetVisible(haveAuthData);

    set_comboBoxCipherText(ckConfig.value(config_key::cipher).
                                    toString(protocols::cloak::defaultCipher));

    set_lineEditSiteText(ckConfig.value(config_key::site).
                                  toString(protocols::cloak::defaultRedirSite));

    set_lineEditPortText(ckConfig.value(config_key::port).
                                  toString(protocols::cloak::defaultPort));

    set_lineEditPortEnabled(container == DockerContainer::Cloak);
}

QJsonObject CloakLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, comboBoxCipherText());
    oldConfig.insert(config_key::site, lineEditSiteText());
    oldConfig.insert(config_key::port, lineEditPortText());

    return oldConfig;
}

void CloakLogic::onPushButtonSaveClicked()
{
    QJsonObject protocolConfig = m_settings->protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Proto::Cloak);
    protocolConfig = getProtocolConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings->containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Proto::Cloak), protocolConfig);

    UiLogic::PageFunc page_func;
    page_func.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    UiLogic::ButtonFunc pushButton_save_func;
    pushButton_save_func.setVisibleFunc = [this] (bool visible) ->void {
        set_pushButtonSaveVisible(visible);
    };
    UiLogic::LabelFunc label_info_func;
    label_info_func.setVisibleFunc = [this] (bool visible) ->void {
        set_labelInfoVisible(visible);
    };
    label_info_func.setTextFunc = [this] (const QString& text) ->void {
        set_labelInfoText(text);
    };
    UiLogic::ProgressFunc progressBar_reset;
    progressBar_reset.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBarResetVisible(visible);
    };
    progressBar_reset.setValueFunc = [this] (int value) ->void {
        set_progressBarResetValue(value);
    };
    progressBar_reset.getValueFunc = [this] (void) -> int {
        return progressBarResetValue();
    };
    progressBar_reset.getMaximiumFunc = [this] (void) -> int {
        return progressBarResetMaximium();
    };

    ErrorCode e = uiLogic()->doInstallAction([this, containerConfig, &newContainerConfig](){
        return m_serverController->updateContainer(m_settings->serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_func, progressBar_reset,
    pushButton_save_func, label_info_func);

    if (!e) {
        m_settings->setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings->clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }

    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}
