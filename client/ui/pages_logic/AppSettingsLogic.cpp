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

#include "ui/qautostart.h"

#include "debug.h"
#include "defines.h"
#include "AppSettingsLogic.h"
#include "utils.h"
#include "vpnconnection.h"
#include <functional>

#if defined Q_OS_MAC || defined Q_OS_LINUX
#include "ui/macos_util.h"
#endif

using namespace amnezia;
using namespace PageEnumNS;


AppSettingsLogic::AppSettingsLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_checkBoxAppSettingsAutostartChecked{false},
    m_checkBoxAppSettingsAutoconnectChecked{false},
    m_checkBoxAppSettingsStartMinimizedChecked{false}
{

}

void AppSettingsLogic::updateAppSettingsPage()
{
    setCheckBoxAppSettingsAutostartChecked(Autostart::isAutostart());
    setCheckBoxAppSettingsAutoconnectChecked(m_settings.isAutoConnect());
    setCheckBoxAppSettingsStartMinimizedChecked(m_settings.isStartMinimized());

    QString ver = QString("%1: %2 (%3)")
            .arg(tr("Software version"))
            .arg(QString(APP_MAJOR_VERSION))
            .arg(__DATE__);
    setLabelAppSettingsVersionText(ver);
}

bool AppSettingsLogic::getCheckBoxAppSettingsAutostartChecked() const
{
    return m_checkBoxAppSettingsAutostartChecked;
}

void AppSettingsLogic::setCheckBoxAppSettingsAutostartChecked(bool checkBoxAppSettingsAutostartChecked)
{
    if (m_checkBoxAppSettingsAutostartChecked != checkBoxAppSettingsAutostartChecked) {
        m_checkBoxAppSettingsAutostartChecked = checkBoxAppSettingsAutostartChecked;
        emit checkBoxAppSettingsAutostartCheckedChanged();
    }
}

bool AppSettingsLogic::getCheckBoxAppSettingsAutoconnectChecked() const
{
    return m_checkBoxAppSettingsAutoconnectChecked;
}

void AppSettingsLogic::setCheckBoxAppSettingsAutoconnectChecked(bool checkBoxAppSettingsAutoconnectChecked)
{
    if (m_checkBoxAppSettingsAutoconnectChecked != checkBoxAppSettingsAutoconnectChecked) {
        m_checkBoxAppSettingsAutoconnectChecked = checkBoxAppSettingsAutoconnectChecked;
        emit checkBoxAppSettingsAutoconnectCheckedChanged();
    }
}

bool AppSettingsLogic::getCheckBoxAppSettingsStartMinimizedChecked() const
{
    return m_checkBoxAppSettingsStartMinimizedChecked;
}

void AppSettingsLogic::setCheckBoxAppSettingsStartMinimizedChecked(bool checkBoxAppSettingsStartMinimizedChecked)
{
    if (m_checkBoxAppSettingsStartMinimizedChecked != checkBoxAppSettingsStartMinimizedChecked) {
        m_checkBoxAppSettingsStartMinimizedChecked = checkBoxAppSettingsStartMinimizedChecked;
        emit checkBoxAppSettingsStartMinimizedCheckedChanged();
    }
}

void AppSettingsLogic::onCheckBoxAppSettingsAutostartToggled(bool checked)
{
    if (!checked) {
        setCheckBoxAppSettingsAutoconnectChecked(false);
    }
    Autostart::setAutostart(checked);
}

void AppSettingsLogic::onCheckBoxAppSettingsAutoconnectToggled(bool checked)
{
    m_settings.setAutoConnect(checked);
}

void AppSettingsLogic::onCheckBoxAppSettingsStartMinimizedToggled(bool checked)
{
    m_settings.setStartMinimized(checked);
}

void AppSettingsLogic::setLabelAppSettingsVersionText(const QString &labelAppSettingsVersionText)
{
    if (m_labelAppSettingsVersionText != labelAppSettingsVersionText) {
        m_labelAppSettingsVersionText = labelAppSettingsVersionText;
        emit labelAppSettingsVersionTextChanged();
    }
}

QString AppSettingsLogic::getLabelAppSettingsVersionText() const
{
    return m_labelAppSettingsVersionText;
}

void AppSettingsLogic::onPushButtonAppSettingsOpenLogsChecked()
{
    Debug::openLogsFolder();
}
