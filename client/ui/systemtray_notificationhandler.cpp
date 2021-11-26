/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemtray_notificationhandler.h"


#ifdef MVPN_MACOS
#  include "platforms/macos/macosutils.h"
#endif

#include <QIcon>
#include <QWindow>


SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject* parent)
    : NotificationHandler(parent) {

//  MozillaVPN* vpn = MozillaVPN::instance();
//  Q_ASSERT(vpn);

//  connect(vpn, &MozillaVPN::stateChanged, this,
//          &SystemTrayNotificationHandler::updateContextMenu);

//  connect(vpn->currentServer(), &ServerData::changed, this,
//          &SystemTrayNotificationHandler::updateContextMenu);

//  connect(vpn->controller(), &Controller::stateChanged, this,
//          &SystemTrayNotificationHandler::updateContextMenu);

//  connect(vpn->statusIcon(), &StatusIcon::iconChanged, this,
//          &SystemTrayNotificationHandler::updateIcon);

//  m_systemTrayIcon.setToolTip(qtTrId("vpn.main.productName"));

//  // Status label
//  m_statusLabel = m_menu.addAction("");
//  m_statusLabel->setEnabled(false);

//  m_lastLocationLabel =
//      m_menu.addAction("", vpn->controller(), &Controller::activate);
//  m_lastLocationLabel->setEnabled(false);

//  m_disconnectAction =
//      m_menu.addAction("", vpn->controller(), &Controller::deactivate);

//  m_separator = m_menu.addSeparator();

//  m_showHideLabel = m_menu.addAction(
//      "", this, &SystemTrayNotificationHandler::showHideWindow);

//  m_menu.addSeparator();

//  m_helpMenu = m_menu.addMenu("");

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

void SystemTrayNotificationHandler::notify(NotificationHandler::Message type,
                                           const QString& title,
                                           const QString& message,
                                           int timerMsec) {
  Q_UNUSED(type);

//  QIcon icon(Constants::LOGO_URL);
//  m_systemTrayIcon.showMessage(title, message, icon, timerMsec);
}

void SystemTrayNotificationHandler::updateIcon(const QString& icon) {
  QIcon trayIconMask(icon);
  trayIconMask.setIsMask(true);
  m_systemTrayIcon.setIcon(trayIconMask);
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

void SystemTrayNotificationHandler::maybeActivated(
    QSystemTrayIcon::ActivationReason reason) {
  qDebug() << "Activated";

#if defined(MVPN_WINDOWS) || defined(MVPN_LINUX)
  if (reason == QSystemTrayIcon::DoubleClick ||
      reason == QSystemTrayIcon::Trigger) {
    QmlEngineHolder* engine = QmlEngineHolder::instance();
    engine->showWindow();
  }
#else
  Q_UNUSED(reason);
#endif
}
