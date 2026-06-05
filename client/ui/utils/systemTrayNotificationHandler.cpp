/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemTrayNotificationHandler.h"

#include "platformTheme.h"
#include "platformTrayTheme.h"
#include "trayIconBackend.h"

#include <QApplication>
#include <QDesktopServices>

#include "version.h"

SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject* parent) :
    NotificationHandler(parent)
{
    m_trayIcon = createTrayIconBackend(this);
    m_trayIcon->setMenu(&m_menu);
    m_trayIcon->setToolTip(APPLICATION_NAME);
    m_trayIcon->setActivatedHandler([this](QSystemTrayIcon::ActivationReason reason) {
        onTrayActivated(reason);
    });

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

    installTrayThemeObserver([this]() { refreshTheme(); }, this);

    m_isDarkTheme = platformIsDarkTheme();
    setTrayState(Vpn::ConnectionState::Disconnected);
    m_trayIcon->show();
}

SystemTrayNotificationHandler::~SystemTrayNotificationHandler() = default;

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
    m_trayActionQuit->setText(tr("Quit") + " " + APPLICATION_NAME);

    if (m_trayIcon) {
        m_trayIcon->rebuildMenu();
    }
}

void SystemTrayNotificationHandler::updateWebsiteUrl(const QString &newWebsiteUrl)
{
    qDebug() << "Updated website URL:" << newWebsiteUrl;
    websiteUrl = newWebsiteUrl;
}

void SystemTrayNotificationHandler::refreshTheme()
{
    const bool isDarkTheme = platformIsDarkTheme();
    const bool themeChanged = (isDarkTheme != m_isDarkTheme);
    m_isDarkTheme = isDarkTheme;

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    updateTrayIcon();
#else
    if (themeChanged) {
        updateTrayIcon();
    }
#endif
}

TrayIconVisual SystemTrayNotificationHandler::currentTrayVisual() const
{
    TrayIconVisual visual;
    visual.connectionState = m_trayState;
    visual.darkTheme = m_isDarkTheme;
    return visual;
}

void SystemTrayNotificationHandler::updateTrayIcon()
{
    if (!m_trayIcon) {
        return;
    }

    m_trayIcon->applyVisual(currentTrayVisual());
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

    if (m_trayIcon) {
        m_trayIcon->rebuildMenu();
    }
}

void SystemTrayNotificationHandler::notify(NotificationHandler::Message type,
                                           const QString& title,
                                           const QString& message,
                                           int timerMsec)
{
    Q_UNUSED(type);

    if (!m_trayIcon) {
        return;
    }

    m_trayIcon->showMessage(title, message, currentTrayVisual(), timerMsec);
}
