/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemtray_notificationhandler.h"


#ifdef Q_OS_MACOS
#  include "platforms/macos/macosutils.h"
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QIcon>
#include <QWindow>

#include "defines.h"


SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject* parent) :
    NotificationHandler(parent),
    m_systemTrayIcon(parent)

{
    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this, &SystemTrayNotificationHandler::onTrayActivated);


    m_menu.addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME, this, [this](){
        emit raiseRequested();
    });
    m_menu.addSeparator();
    m_trayActionConnect = m_menu.addAction(tr("Connect"), this, [this](){ emit connectRequested(); });
    m_trayActionDisconnect = m_menu.addAction(tr("Disconnect"), this, [this](){ emit disconnectRequested(); });

    m_menu.addSeparator();

    m_menu.addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl("https://amnezia.org"));
    });

    m_menu.addAction(QIcon(":/images/tray/cancel.png"), tr("Quit") + " " + APPLICATION_NAME, this, [&](){
        qApp->quit();
    });

    m_systemTrayIcon.setContextMenu(&m_menu);
    setTrayState(VpnProtocol::Disconnected);


//  m_preferencesAction = m_menu.addAction("", vpn, &MozillaVPN::requestSettings);

//  m_menu.addSeparator();

//  m_quitAction = m_menu.addAction("", vpn->controller(), &Controller::quit);
//  m_systemTrayIcon.setContextMenu(&m_menu);

//  updateIcon(vpn->statusIcon()->iconString());

//  connect(QmlEngineHolder::instance()->window(), &QWindow::visibleChanged, this,
//          &SystemTrayNotificationHandler::updateContextMenu);

//  connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this,
//          &SystemTrayNotificationHandler::maybeActivated);

//  connect(&m_systemTrayIcon, &QSystemTrayIcon::messageClicked, this,
//          &SystemTrayNotificationHandler::messageClickHandle);

//  retranslate();

//  m_systemTrayIcon.show();
}

SystemTrayNotificationHandler::~SystemTrayNotificationHandler() {
}

void SystemTrayNotificationHandler::setConnectionState(VpnProtocol::VpnConnectionState state)
{
    setTrayState(state);
    NotificationHandler::setConnectionState(state);
}

void SystemTrayNotificationHandler::setTrayIcon(const QString &iconPath)
{
    QIcon trayIconMask(QPixmap(iconPath).scaled(128,128));
    trayIconMask.setIsMask(true);
    m_systemTrayIcon.setIcon(trayIconMask);
}

void SystemTrayNotificationHandler::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
#ifndef Q_OS_MAC
    if(reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        emit raiseRequested();
    }
#endif
}

void SystemTrayNotificationHandler::setTrayState(VpnProtocol::VpnConnectionState state)
{
    qDebug() << "SystemTrayNotificationHandler::setTrayState" << state;
    QString resourcesPath = ":/images/tray/%1";

    switch (state) {
    case VpnProtocol::Disconnected:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case VpnProtocol::Preparing:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case VpnProtocol::Connecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case VpnProtocol::Connected:
        setTrayIcon(QString(resourcesPath).arg(ConnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case VpnProtocol::Disconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case VpnProtocol::Reconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case VpnProtocol::Error:
        setTrayIcon(QString(resourcesPath).arg(ErrorTrayIconName));
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case VpnProtocol::Unknown:
    default:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
    }

    //#ifdef Q_OS_MAC
    //    // Get theme from current user (note, this app can be launched as root application and in this case this theme can be different from theme of real current user )
    //    bool darkTaskBar = MacOSFunctions::instance().isMenuBarUseDarkTheme();
    //    darkTaskBar = forceUseBrightIcons ? true : darkTaskBar;
    //    resourcesPath = ":/images_mac/tray_icon/%1";
    //    useIconName = useIconName.replace(".png", darkTaskBar ? "@2x.png" : " dark@2x.png");
    //#endif
}


void SystemTrayNotificationHandler::notify(NotificationHandler::Message type,
                                           const QString& title,
                                           const QString& message,
                                           int timerMsec) {
  Q_UNUSED(type);

  QIcon icon(ConnectedTrayIconName);
  m_systemTrayIcon.showMessage(title, message, icon, timerMsec);
}

void SystemTrayNotificationHandler::showHideWindow() {
//  QmlEngineHolder* engine = QmlEngineHolder::instance();
//  if (engine->window()->isVisible()) {
//    engine->hideWindow();
//#ifdef MVPN_MACOS
//    MacOSUtils::hideDockIcon();
//#endif
//  } else {
//    engine->showWindow();
//#ifdef MVPN_MACOS
//    MacOSUtils::showDockIcon();
//#endif
//  }
}

