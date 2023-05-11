#include "ServerListLogic.h"

#include "vpnconnection.h"
#include "../models/servers_model.h"
#include "../uilogic.h"

ServerListLogic::ServerListLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_serverListModel{new ServersModel(m_settings, this)}
{

}

void ServerListLogic::onServerListPushbuttonDefaultClicked(int index)
{
    m_settings->setDefaultServer(index);
    uiLogic()->onUpdateAllPages();
    emit currServerIdxChanged();
}

void ServerListLogic::onServerListPushbuttonSettingsClicked(int index)
{
    uiLogic()->m_selectedServerIndex = index;
    uiLogic()->goToPage(Page::ServerSettings);
}

int ServerListLogic::currServerIdx() const
{
    return m_settings->defaultServerIndex();
}

void ServerListLogic::onUpdatePage()
{
    const QJsonArray &servers = m_settings->serversArray();
    int defaultServer = m_settings->defaultServerIndex();
    QVector<ServerModelContent> serverListContent;
    for(int i = 0; i < servers.size(); i++) {
        ServerModelContent c;
        auto server = servers.at(i).toObject();
        c.desc = server.value(config_key::description).toString();
        c.address = server.value(config_key::hostName).toString();
        if (c.desc.isEmpty()) {
            c.desc = c.address;
        }
        c.isDefault = (i == defaultServer);
        serverListContent.push_back(c);
    }
//    qobject_cast<ServersModel*>(m_serverListModel)->setContent(serverListContent);
}
