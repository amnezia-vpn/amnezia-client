/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "systemTrayNotificationHandler.h"
#include <QDebug>

#ifdef Q_OS_MAC
    #include "platforms/macos/macosstatusicon.h"
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QIcon>
#include <QPainter>
#include <QStyleHints>
#include <QWindow>
#include <QtMath>

#include "core/protocols/vpnProtocol.h"

#ifdef Q_OS_WIN
    #include <QSettings>
#endif

#include "version.h"

SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject *parent)
    : NotificationHandler(parent)
#ifndef Q_OS_MAC
      ,
      m_systemTrayIcon(parent)
#endif
{
    m_trayActionShow = m_menu.addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME,
                                        this, [this]() { emit raiseRequested(); });
    m_menu.addSeparator();
    m_trayActionConnect = m_menu.addAction(tr("Connect"), this, [this]() { emit connectRequested(); });
    m_trayActionDisconnect = m_menu.addAction(tr("Disconnect"), this, [this]() { emit disconnectRequested(); });

    m_menu.addSeparator();

    m_trayActionVisitWebSite = m_menu.addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"),
                                                [&]() { QDesktopServices::openUrl(QUrl(websiteUrl)); });

    // Quit action: disconnect VPN first on macOS NE, else quit directly
    m_trayActionQuit = m_menu.addAction(QIcon(":/images/tray/cancel.png"), tr("Quit") + " " + APPLICATION_NAME, this,
                                        [&]() { qApp->quit(); });

#ifdef Q_OS_MAC
    // QSystemTrayIcon::setContextMenu crashes on macOS 14+: its menu-tracking
    // observer reads -[NSEvent clickCount] off a non-mouse event. Own the
    // NSStatusItem and attach the native NSMenu instead.
    m_statusIcon = new MacOSStatusIcon(this);
    m_statusIcon->setMenu(&m_menu);
#else
    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this, &SystemTrayNotificationHandler::onTrayActivated);
    m_systemTrayIcon.setContextMenu(&m_menu);
#endif
    m_trayIconBase = QImage(":/images/tray/default.png").convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_pulseTimer.setInterval(60);
    connect(&m_pulseTimer, &QTimer::timeout, this, &SystemTrayNotificationHandler::updateTrayIcon);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &SystemTrayNotificationHandler::updateTrayIcon);
    setTrayState(Vpn::ConnectionState::Disconnected);
}

SystemTrayNotificationHandler::~SystemTrayNotificationHandler()
{
#ifdef Q_OS_MAC
    delete m_statusIcon; // before m_menu: the status item references its NSMenu
    m_statusIcon = nullptr;
#endif
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
    m_trayActionQuit->setText(tr("Quit") + " " + APPLICATION_NAME);
    updateTrayToolTip();
}

void SystemTrayNotificationHandler::updateWebsiteUrl(const QString &newWebsiteUrl)
{
    qDebug() << "Updated website URL:" << newWebsiteUrl;
    websiteUrl = newWebsiteUrl;
}

QColor SystemTrayNotificationHandler::trayIconColor() const
{
#ifdef Q_OS_WIN
    QSettings personalize("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                          QSettings::NativeFormat);
    return personalize.value("SystemUsesLightTheme", 0).toInt() == 1 ? Qt::black : Qt::white;
#else
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light ? Qt::black : Qt::white;
#endif
}

void SystemTrayNotificationHandler::updateTrayIcon()
{
    const bool isConnected = m_trayState == Vpn::ConnectionState::Connected;
    const bool isError = m_trayState == Vpn::ConnectionState::Error;
    const bool isColored = isConnected || isError;

    QColor color = isConnected ? ConnectedTrayIconColor : isError ? ErrorTrayIconColor : trayIconColor();
    if (m_pulseTimer.isActive()) {
        const qreal phase = (m_pulseClock.elapsed() % PulsePeriodMs) / qreal(PulsePeriodMs);
        color.setAlphaF(0.65 + 0.35 * qCos(2 * M_PI * phase));
    } else if (!isColored) {
        color.setAlphaF(0.5);
    }

    QImage icon = m_trayIconBase;
    QPainter painter(&icon);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(icon.rect(), color);
    painter.end();

    setTrayIcon(icon, !isColored);
}

void SystemTrayNotificationHandler::setTrayIcon(const QImage &icon, bool isTemplate)
{
#ifdef Q_OS_MAC
    m_statusIcon->setIcon(icon, isTemplate);
#else
    Q_UNUSED(isTemplate);
    m_systemTrayIcon.setIcon(QIcon(QPixmap::fromImage(icon)));
#endif
}

void SystemTrayNotificationHandler::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        emit raiseRequested();
    }
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
    default: m_trayActionConnect->setEnabled(false); m_trayActionDisconnect->setEnabled(true);
    }

    const bool isPulsing = state == Vpn::ConnectionState::Preparing || state == Vpn::ConnectionState::Connecting
            || state == Vpn::ConnectionState::Reconnecting;
    if (!isPulsing) {
        m_pulseTimer.stop();
    } else if (!m_pulseTimer.isActive()) {
        m_pulseClock.start();
        m_pulseTimer.start();
    }

    updateTrayIcon();
    updateTrayToolTip();
}

void SystemTrayNotificationHandler::updateTrayToolTip()
{
    const QString toolTip = QString("%1 — %2").arg(APPLICATION_NAME, VpnProtocol::textConnectionState(m_trayState));
#ifdef Q_OS_MAC
    m_statusIcon->setToolTip(toolTip);
#else
    m_systemTrayIcon.setToolTip(toolTip);
#endif
}

void SystemTrayNotificationHandler::notify(NotificationHandler::Message type, const QString &title,
                                           const QString &message, int timerMsec)
{
    Q_UNUSED(type);

#ifdef Q_OS_MAC
    Q_UNUSED(timerMsec);
    m_statusIcon->showMessage(title, message);
#else
    m_systemTrayIcon.showMessage(title, message, QIcon(), timerMsec);
#endif
}

void SystemTrayNotificationHandler::showHideWindow()
{
    //  QmlEngineHolder* engine = QmlEngineHolder::instance();
    //  if (engine->window()->isVisible()) {
    //    engine->hideWindow();
    // #ifdef MVPN_MACOS
    //    MacOSUtils::hideDockIcon();
    // #endif
    //  } else {
    //    engine->showWindow();
    // #ifdef MVPN_MACOS
    //    MacOSUtils::showDockIcon();
    // #endif
    //  }
}
