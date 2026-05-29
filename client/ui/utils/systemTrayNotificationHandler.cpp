/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemTrayNotificationHandler.h"

#include <QApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QWindow>

#include "version.h"

namespace {

constexpr int kTrayIconSize = 128;
constexpr char kTrayTemplateIconPath[] = ":/images/tray/icon.svg";

QPixmap renderTrayTemplate(const QString &resourcePath, qreal opacity)
{
    QSvgRenderer renderer(resourcePath);
    QPixmap pixmap(kTrayIconSize, kTrayIconSize);
    pixmap.fill(Qt::transparent);

    if (!renderer.isValid()) {
        qWarning() << "Failed to load tray icon template:" << resourcePath;
        return pixmap;
    }

    QPainter painter(&pixmap);
    painter.setOpacity(opacity);
    renderer.render(&painter, QRectF(0, 0, kTrayIconSize, kTrayIconSize));
    return pixmap;
}

QIcon buildTrayIcon(qreal opacity)
{
    const QPixmap pixmap = renderTrayTemplate(QString::fromLatin1(kTrayTemplateIconPath), opacity);

    QIcon icon;
    icon.addPixmap(pixmap);
    icon.setIsMask(true);
    return icon;
}

} // namespace

SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject* parent) :
    NotificationHandler(parent),
    m_systemTrayIcon(parent)

{
    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this, &SystemTrayNotificationHandler::onTrayActivated);

    m_trayActionShow =  m_menu.addAction(tr("Show") + " " + APPLICATION_NAME, this, [this](){
        emit raiseRequested();
    });
    m_menu.addSeparator();
    m_trayActionConnect = m_menu.addAction(tr("Connect"), this, [this](){ emit connectRequested(); });
    m_trayActionDisconnect = m_menu.addAction(tr("Disconnect"), this, [this](){ emit disconnectRequested(); });

    m_menu.addSeparator();

    m_trayActionVisitWebSite = m_menu.addAction(tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl(websiteUrl));
    });

    m_trayActionQuit = m_menu.addAction(tr("Quit") + " " + APPLICATION_NAME,
                                       this,
                                       [&](){ qApp->quit(); });

    m_systemTrayIcon.setContextMenu(&m_menu);

    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        connect(styleHints, &QStyleHints::colorSchemeChanged, this, [this]() {
            updateTrayIcon();
        });
    }

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

void SystemTrayNotificationHandler::updateWebsiteUrl(const QString &newWebsiteUrl) {
    qDebug() << "Updated website URL:" << newWebsiteUrl;
    websiteUrl = newWebsiteUrl;
}

qreal SystemTrayNotificationHandler::trayIconOpacityForState(Vpn::ConnectionState state) const
{
    switch (state) {
    case Vpn::ConnectionState::Connected:
    case Vpn::ConnectionState::Error:
        return kConnectedTrayOpacity;
    case Vpn::ConnectionState::Disconnected:
    case Vpn::ConnectionState::Preparing:
    case Vpn::ConnectionState::Connecting:
    case Vpn::ConnectionState::Disconnecting:
    case Vpn::ConnectionState::Reconnecting:
    case Vpn::ConnectionState::Unknown:
    default:
        return kDisconnectedTrayOpacity;
    }
}

void SystemTrayNotificationHandler::updateTrayIcon()
{
    const qreal opacity = trayIconOpacityForState(m_trayState);
    m_systemTrayIcon.setIcon(buildTrayIcon(opacity));
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
    m_trayState = state;

    switch (state) {
    case Vpn::ConnectionState::Disconnected:
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case Vpn::ConnectionState::Preparing:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Connecting:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Connected:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Disconnecting:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Reconnecting:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Error:
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case Vpn::ConnectionState::Unknown:
    default:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    }

    updateTrayIcon();
}


void SystemTrayNotificationHandler::notify(NotificationHandler::Message type,
                                           const QString& title,
                                           const QString& message,
                                           int timerMsec) {
  Q_UNUSED(type);

  m_systemTrayIcon.showMessage(title, message, buildTrayIcon(kConnectedTrayOpacity), timerMsec);
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
