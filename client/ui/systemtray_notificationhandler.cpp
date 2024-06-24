/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemtray_notificationhandler.h"


#ifdef Q_OS_MAC
#  include "platforms/macos/macosutils.h"
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QIcon>
#include <QWindow>

#include "version.h"

SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject* parent) :
    NotificationHandler(parent),
    m_systemTrayIcon(parent)

{
    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this, &SystemTrayNotificationHandler::onTrayActivated);

    m_trayActionShow =  m_menu.addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME, this, [this](){
        emit raiseRequested();
    });
    m_menu.addSeparator();
    m_trayActionConnect = m_menu.addAction(tr("Connect"), this, [this](){ emit connectRequested(); });
    m_trayActionDisconnect = m_menu.addAction(tr("Disconnect"), this, [this](){ emit disconnectRequested(); });

    m_menu.addSeparator();

    m_trayActionVisitWebSite = m_menu.addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl("https://naruzhu.click/appam"));
    });

    m_trayActionQuit = m_menu.addAction(QIcon(":/images/tray/cancel.png"), tr("Quit") + " " + APPLICATION_NAME, this, [&](){
        qApp->quit();
    });

    m_systemTrayIcon.setContextMenu(&m_menu);
    setTrayState(Vpn::ConnectionState::Disconnected);
}

SystemTrayNotificationHandler::~SystemTrayNotificationHandler() {
}

void SystemTrayNotificationHandler::setConnectionState(Vpn::ConnectionState state)
{
    setTrayState(state);
    NotificationHandler::setConnectionState(state);
}

void SystemTrayNotificationHandler::onTranslationsUpdated()
{
    m_trayActionShow->setText(tr("Show") + " " + APPLICATION_NAME);
    m_trayActionConnect->setText(tr("Connect"));
    m_trayActionDisconnect->setText(tr("Disconnect"));
    m_trayActionVisitWebSite->setText(tr("Visit Website"));
    m_trayActionQuit->setText(tr("Quit")+ " " + APPLICATION_NAME);
}

void SystemTrayNotificationHandler::setTrayIcon(const QString &iconPath)
{
    QIcon trayIconMask(QPixmap(iconPath).scaled(128,128));
#ifndef Q_OS_MAC
    trayIconMask.setIsMask(true);
#endif
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

void SystemTrayNotificationHandler::setTrayState(Vpn::ConnectionState state)
{
    QString resourcesPath = ":/images/tray/%1";

    switch (state) {
    case Vpn::ConnectionState::Disconnected:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case Vpn::ConnectionState::Preparing:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Connecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Connected:
        setTrayIcon(QString(resourcesPath).arg(ConnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Disconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Reconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Error:
        setTrayIcon(QString(resourcesPath).arg(ErrorTrayIconName));
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case Vpn::ConnectionState::Unknown:
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

