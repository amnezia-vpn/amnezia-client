#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QRegularExpression>
#include <QSaveFile>

//#include "configurators/cloak_configurator.h"
//#include "configurators/vpn_configurator.h"
//#include "configurators/openvpn_configurator.h"
//#include "configurators/shadowsocks_configurator.h"
//#include "configurators/ssh_configurator.h"

//#include "core/servercontroller.h"
//#include "core/server_defs.h"
//#include "core/errorstrings.h"

//#include "protocols/protocols_defs.h"
//#include "protocols/shadowsocksvpnprotocol.h"

#include "debug.h"
#include "defines.h"
#include "ServerListLogic.h"
#include "utils.h"
#include "vpnconnection.h"
#include <functional>

#include "../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;


ServerListLogic::ServerListLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_serverListModel{new ServersModel(this)}
{

}

QObject* ServerListLogic::getServerListModel() const
{
    return m_serverListModel;
}

void ServerListLogic::onServerListPushbuttonDefaultClicked(int index)
{
    m_settings.setDefaultServer(index);
    updateServersListPage();
}

void ServerListLogic::onServerListPushbuttonSettingsClicked(int index)
{
    m_uiLogic->selectedServerIndex = index;
    m_uiLogic->goToPage(Page::ServerSettings);
}

void ServerListLogic::updateServersListPage()
{
    const QJsonArray &servers = m_settings.serversArray();
    int defaultServer = m_settings.defaultServerIndex();
    std::vector<ServerModelContent> serverListContent;
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
    m_serverListModel->setContent(serverListContent);
}
