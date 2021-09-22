#include "OtherProtocolsLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

OtherProtocolsLogic::OtherProtocolsLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent)
{

}

void OtherProtocolsLogic::updateProtocolPage(const QJsonObject &config, DockerContainer container, bool haveAuthData)
{
    set_labelTftpUserNameText(config.value(config_key::userName).toString());
    set_labelTftpPasswordText(config.value(config_key::password).toString(protocols::sftp::defaultUserName));
    set_labelTftpPortText(config.value(config_key::port).toString(protocols::sftp::defaultUserName));
}

//QJsonObject OtherProtocolsLogic::getProtocolConfigFromPage(QJsonObject oldConfig)
//{

//}

void OtherProtocolsLogic::onPushButtonProtoShadowSocksSaveClicked()
{

}
