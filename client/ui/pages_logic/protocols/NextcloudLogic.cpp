#include "NextcloudLogic.h"

#include <functional>

#include "core/servercontroller.h"
#include "ui/pages_logic/ServerConfiguringProgressLogic.h"
#include "ui/uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

NextcloudLogic::NextcloudLogic(UiLogic *logic, QObject *parent):
      PageProtocolLogicBase(logic, parent),
      m_lineAdminUserText{},
      m_lineAdminPasswordText{},
      m_pushButtonSaveVisible{false},
      m_progressBarResetVisible{false},
      m_labelInfoVisible{true},
      m_labelInfoText{},
      m_progressBarResetValue{0},
      m_progressBarResetMaximium{100}

{

}

void NextcloudLogic::updateProtocolPage(const QJsonObject &nextcloudConfig, DockerContainer container, bool haveAuthData)
{
    set_pageEnabled(haveAuthData);
    set_pushButtonSaveVisible(haveAuthData);
    set_progressBarResetVisible(haveAuthData);

    set_lineAdminUserText(nextcloudConfig.value(config_key::adminUser).toString(protocols::nextcloud::defaultAdminUser));
    set_lineAdminPasswordText(nextcloudConfig.value(config_key::adminPassword).toString(protocols::nextcloud::defaultAdminPassword));
}

QJsonObject NextcloudLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::adminUser, lineAdminUserText());
    oldConfig.insert(config_key::adminPassword, lineAdminPasswordText());
    return oldConfig;
}

void NextcloudLogic::onPushButtonSaveClicked()
{
    QJsonObject protocolConfig = m_settings->protocolConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer, Proto::Nextcloud);
    protocolConfig = getProtocolConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings->containerConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(ProtocolProps::protoToString(Proto::Nextcloud), protocolConfig);
    /*

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
        ServerController serverController(m_settings);
        return serverController.updateContainer(m_settings->serverCredentials(uiLogic()->m_selectedServerIndex),
                                                uiLogic()->m_selectedDockerContainer, containerConfig, newContainerConfig);
    };

    ErrorCode e = uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->doInstallAction(installAction, pageFunc, progressBarFunc,
                                                                                          saveButtonFunc, waitInfoFunc,
                                                                                          busyInfoFuncy, cancelButtonFunc);

    if (!e) {
        m_settings->setContainerConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer, newContainerConfig);
        m_settings->clearLastConnectionConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << uiLogic()->m_selectedServerIndex << uiLogic()->m_selectedDockerContainer;
*/

}

void NextcloudLogic::onPushButtonCancelClicked()
{
    emit uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->cancelDoInstallAction(true);
}
