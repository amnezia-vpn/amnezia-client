#include "V2RayLogic.h"

#include <functional>

#include "core/servercontroller.h"
#include "ui/pages_logic/ServerConfiguringProgressLogic.h"
#include "ui/uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

V2RayLogic::V2RayLogic(UiLogic *logic, QObject *parent):
      PageProtocolLogicBase(logic, parent),
      m_lineEditPortText{},
      m_pushButtonSaveVisible{false},
      m_progressBarResetVisible{false},
      m_lineEditPortEnabled{false},
      m_labelInfoVisible{true},
      m_labelInfoText{},
      m_progressBarResetValue{0},
      m_progressBarResetMaximium{100}
{

}

void V2RayLogic::updateProtocolPage(const QJsonObject &v2RayConfig, DockerContainer container, bool haveAuthData)
{
    set_pageEnabled(haveAuthData);
    set_pushButtonSaveVisible(haveAuthData);
    set_progressBarResetVisible(haveAuthData);

    set_lineEditPortText(v2RayConfig.value(config_key::port).
                         toString(protocols::v2ray::defaultServerPort));

    set_lineEditPortEnabled(container == DockerContainer::V2Ray);
}

QJsonObject V2RayLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::port, lineEditPortText());
    return oldConfig;
}

void V2RayLogic::onPushButtonSaveClicked()
{
    QJsonObject protocolConfig = m_settings->protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Proto::V2Ray);

    QJsonObject containerConfig = m_settings->containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Proto::V2Ray), protocolConfig);
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
    progressBarFunc.getMaximiumFunc = [this] (void) -> int {
        return progressBarResetMaximium();
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

    auto installAction = [this, containerConfig, &newContainerConfig]() {
        return m_serverController->updateContainer(m_settings->serverCredentials(uiLogic()->selectedServerIndex),
                                                   uiLogic()->selectedDockerContainer,
                                                   containerConfig,
                                                   newContainerConfig);
    };

    ErrorCode e = uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->doInstallAction(installAction, pageFunc, progressBarFunc,
                                                                                          saveButtonFunc, waitInfoFunc,
                                                                                          busyInfoFuncy, cancelButtonFunc);

    if (!e) {
        m_settings->setContainerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, newContainerConfig);
        m_settings->clearLastConnectionConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->selectedServerIndex << uiLogic()->selectedDockerContainer;
}

void V2RayLogic::onPushButtonCancelClicked()
{
    emit uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->cancelDoInstallAction(true);
}
