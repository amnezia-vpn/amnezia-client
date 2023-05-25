#include "NextcloudLogic.h"

#include <functional>

#include "core/servercontroller.h"
#include "ui/pages_logic/ServerConfiguringProgressLogic.h"
#include "ui/uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

NextcloudLogic::NextcloudLogic(UiLogic *logic, QObject *parent):
      PageProtocolLogicBase(logic, parent),
      m_lineNextcloudPortText{},
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
}

void NextcloudLogic::onPushButtonCancelClicked()
{
    emit uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->cancelDoInstallAction(true);
}
