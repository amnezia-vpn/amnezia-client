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
#include "ServerSettingsLogic.h"
#include "utils.h"
#include "vpnconnection.h"
#include <functional>

#if defined Q_OS_MAC || defined Q_OS_LINUX
#include "ui/macos_util.h"
#endif

using namespace amnezia;
using namespace PageEnumNS;


ServerSettingsLogic::ServerSettingsLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic)
{

}
