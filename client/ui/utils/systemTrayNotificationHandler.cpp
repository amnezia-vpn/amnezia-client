/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemTrayNotificationHandler.h"

#include "platformTheme.h"

#include <QApplication>
#include <QBuffer>
#include <QColor>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QEvent>
#include <functional>

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
#  include "platforms/macos/macosstatusicon.h"
#endif

#include "version.h"

namespace {

constexpr int kTrayIconSize = 128;
constexpr char kTrayTemplateIconPath[] = ":/images/tray/icon.svg";

class TrayThemeChangeFilter final : public QObject {
public:
    explicit TrayThemeChangeFilter(std::function<void()> onThemeChanged, QObject *parent = nullptr)
        : QObject(parent)
        , m_onThemeChanged(std::move(onThemeChanged))
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        Q_UNUSED(watched);
        if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ThemeChange) {
            if (m_onThemeChanged) {
                m_onThemeChanged();
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void()> m_onThemeChanged;
};

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

QByteArray renderTrayTemplatePng(qreal opacity)
{
    const QPixmap pixmap = renderTrayTemplate(QString::fromLatin1(kTrayTemplateIconPath), opacity);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return bytes;
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
    NotificationHandler(parent)
#if !defined(Q_OS_MAC) || defined(MACOS_NE)
    , m_systemTrayIcon(parent)
#endif
{
#if defined(Q_OS_MAC) && !defined(MACOS_NE)
    m_macStatusIcon = new MacOSStatusIcon(this);
    m_macStatusIcon->setMenu(&m_menu);
    m_macStatusIcon->setToolTip(APPLICATION_NAME);
    // Template NSStatusItem icons follow the menu bar appearance automatically.
#else
    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this, &SystemTrayNotificationHandler::onTrayActivated);
    m_systemTrayIcon.setContextMenu(&m_menu);
#endif

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

#if !defined(Q_OS_MAC) || defined(MACOS_NE)
    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        connect(styleHints, &QStyleHints::colorSchemeChanged, this, [this]() {
            refreshTheme();
        });
    }

    qApp->installEventFilter(new TrayThemeChangeFilter([this]() {
        refreshTheme();
    }, this));
#endif

    refreshTheme();
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

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
    if (m_macStatusIcon) {
        m_macStatusIcon->rebuildNativeMenu();
    }
#endif
}

void SystemTrayNotificationHandler::updateWebsiteUrl(const QString &newWebsiteUrl) {
    qDebug() << "Updated website URL:" << newWebsiteUrl;
    websiteUrl = newWebsiteUrl;
}

void SystemTrayNotificationHandler::refreshTheme()
{
    m_isDarkTheme = platformIsDarkTheme();

#if !defined(Q_OS_MAC) || defined(MACOS_NE)
    updateTrayIcon();
#endif
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

QColor SystemTrayNotificationHandler::trayIndicatorColorForState(Vpn::ConnectionState state) const
{
    switch (state) {
    case Vpn::ConnectionState::Connected:
        return QColor(52, 199, 89);
    case Vpn::ConnectionState::Error:
        return QColor(235, 87, 87);
    default:
        return QColor();
    }
}

void SystemTrayNotificationHandler::updateTrayIcon()
{
    const qreal opacity = trayIconOpacityForState(m_trayState);

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
    Q_ASSERT(m_macStatusIcon);
    m_macStatusIcon->setIconFromData(renderTrayTemplatePng(opacity));
    m_macStatusIcon->setIndicatorColor(trayIndicatorColorForState(m_trayState));
#else
    m_systemTrayIcon.setIcon(buildTrayIcon(opacity));
#endif
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

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
    if (m_macStatusIcon) {
        m_macStatusIcon->rebuildNativeMenu();
    }
#endif
}

void SystemTrayNotificationHandler::notify(NotificationHandler::Message type,
                                           const QString& title,
                                           const QString& message,
                                           int timerMsec) {
  Q_UNUSED(type);

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
  Q_ASSERT(m_macStatusIcon);
  m_macStatusIcon->showMessage(title, message);
#else
  m_systemTrayIcon.showMessage(title, message, buildTrayIcon(kConnectedTrayOpacity), timerMsec);
#endif
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
