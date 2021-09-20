#include "NewServerProtocolsLogic.h"
#include "../uilogic.h"

NewServerProtocolsLogic::NewServerProtocolsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_progressBarConnectionMinimum{0},
    m_progressBarConnectionMaximum{100}
{
}


void NewServerProtocolsLogic::updatePage()
{
    set_progressBarConnectionMinimum(0);
    set_progressBarConnectionMaximum(300);
}

void NewServerProtocolsLogic::onPushButtonConfigureClicked(DockerContainer c, int port, TransportProto tp)
{
    QMap<DockerContainer, QJsonObject> containers;
    Protocol mainProto = ContainerProps::defaultProtocol(c);

    QJsonObject config {
        { config_key::container, ContainerProps::containerToString(c) },
        { ProtocolProps::protoToString(mainProto), QJsonObject {
                { config_key::port, QString::number(port) },
                { config_key::transport_proto, ProtocolProps::transportProtoToString(tp, mainProto) }}
        }
    };

    containers.insert(c, config);

    uiLogic()->installServer(containers);
}

