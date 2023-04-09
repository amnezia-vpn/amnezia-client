#include "CloakLogic.h"

#include <functional>

#include "core/servercontroller.h"
#include "ui/uilogic.h"
#include "ui/pages_logic/ServerConfiguringProgressLogic.h"

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
    m_progressBarResetMaximum{100}
{

}

void CloakLogic::updateProtocolPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData)
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
    QJsonObject protocolConfig = m_settings->protocolConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer, Proto::Cloak);
    protocolConfig = getProtocolConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings->containerConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Proto::Cloak), protocolConfig);

    ServerConfiguringProgressLogic::PageFunc pageFunc;
    pageFunc.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    ServerConfiguringProgressLogic::ButtonFunc saveButtonFunc;
    saveButtonFunc.setVisibleFunc = [this] (bool visible) -> void {
        set_pushButtonSaveVisible(visible);
    };
    ServerConfiguringProgressLogic::LabelFunc waitInfoFunc;
    waitInfoFunc.setVisibleFunc = [this] (bool visible) -> void {
        set_labelInfoVisible(visible);
    };
    waitInfoFunc.setTextFunc = [this] (const QString& text) -> void {
        set_labelInfoText(text);
    };
    ServerConfiguringProgressLogic::ProgressFunc progressBarFunc;
    progressBarFunc.setVisibleFunc = [this] (bool visible) -> void {
        set_progressBarResetVisible(visible);
    };
    progressBarFunc.setValueFunc = [this] (int value) -> void {
        set_progressBarResetValue(value);
    };
    progressBarFunc.getValueFunc = [this] (void) -> int {
        return progressBarResetValue();
    };
    progressBarFunc.getMaximumFunc = [this] (void) -> int {
        return progressBarResetMaximum();
    };
    progressBarFunc.setTextVisibleFunc = [this] (bool visible) -> void {
        set_progressBarTextVisible(visible);
    };
    progressBarFunc.setTextFunc = [this] (const QString& text) -> void {
        set_progressBarText(text);
    };

    ServerConfiguringProgressLogic::LabelFunc busyInfoFuncy;
    busyInfoFuncy.setTextFunc = [this] (const QString& text) -> void {
        set_labelServerBusyText(text);
    };
    busyInfoFuncy.setVisibleFunc = [this] (bool visible) -> void {
        set_labelServerBusyVisible(visible);
    };

    ServerConfiguringProgressLogic::ButtonFunc cancelButtonFunc;
    cancelButtonFunc.setVisibleFunc = [this] (bool visible) -> void {
        set_pushButtonCancelVisible(visible);
    };

    progressBarFunc.setTextVisibleFunc(true);
    progressBarFunc.setTextFunc(QString("Configuring..."));
    ErrorCode e = uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->doInstallAction([this, containerConfig, &newContainerConfig](){
        return m_serverController->updateContainer(m_settings->serverCredentials(uiLogic()->m_selectedServerIndex),
                                                   uiLogic()->m_selectedDockerContainer,
                                                   containerConfig,
                                                   newContainerConfig);
    },
    pageFunc, progressBarFunc,
    saveButtonFunc, waitInfoFunc,
    busyInfoFuncy, cancelButtonFunc);

    if (!e) {
        m_settings->setContainerConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer, newContainerConfig);
        m_settings->clearLastConnectionConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer);
    }

    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->m_selectedServerIndex << uiLogic()->m_selectedDockerContainer;
}

void CloakLogic::onPushButtonCancelClicked()
{
    emit uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->cancelDoInstallAction(true);
}
