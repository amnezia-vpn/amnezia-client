/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include "systemtray_notificationhandler.h"


#ifdef Q_OS_MAC
#  include "platforms/macos/macosutils.h"
#endif

#include <QApplication>
#include <QBuffer>
#include <QDesktopServices>
#include <QIcon>
#include <QPainter>
#include <QWindow>

#ifdef Q_OS_MACOS
#  include "platforms/macos/macosstatusicon.h"
#endif

#include "version.h"

namespace
{
#ifdef Q_OS_MACOS
struct MacMenuBarIcon
{
    QPixmap pixmap;
    qreal itemLength;
    qreal imageWidth;
    qreal imageHeight;
};

QByteArray encodePixmapAsPng(const QPixmap& pixmap)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return bytes;
}

MacMenuBarIcon buildMacMenuBarVpnIcon(bool active)
{
    constexpr int canvasWidth = 52;
    constexpr int canvasHeight = 88;
    constexpr qreal borderInsetX = 8.0;
    constexpr qreal borderInsetY = 2.0;
    constexpr qreal borderWidth = 4.0;
    constexpr qreal borderRadius = 4.5;
    const QColor strokeColor = active ? QColor(255, 255, 255) : QColor(160, 164, 170);

    QPixmap pixmap(canvasWidth, canvasHeight);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen borderPen(strokeColor);
    borderPen.setWidthF(borderWidth);
    borderPen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    const QRectF borderRect(borderInsetX + borderWidth * 0.5,
                            borderInsetY + borderWidth * 0.5,
                            canvasWidth - borderInsetX * 2.0 - borderWidth,
                            canvasHeight - borderInsetY * 2.0 - borderWidth);
    painter.drawRoundedRect(borderRect, borderRadius, borderRadius);

    QFont font(qApp->font());
    font.setBold(true);
    font.setWeight(QFont::Bold);
    font.setPixelSize(18);
    painter.setFont(font);
    painter.setPen(strokeColor);

    const QStringList letters = {QStringLiteral("V"), QStringLiteral("P"), QStringLiteral("N")};
    const QRectF letterRect = borderRect.adjusted(2.0, 4.0, -2.0, -4.0);
    const qreal rowHeight = letterRect.height() / letters.size();

    for (int i = 0; i < letters.size(); ++i) {
        const qreal y = letterRect.top() + i * rowHeight;
        painter.drawText(QRectF(letterRect.left(), y, letterRect.width(), rowHeight),
                         Qt::AlignCenter,
                         letters.at(i));
    }

    painter.end();
    return {pixmap, 14.0, 12.0, 19.0};
}
#endif
}

SystemTrayNotificationHandler::SystemTrayNotificationHandler(QObject* parent) :
    NotificationHandler(parent),
    m_systemTrayIcon(parent)

{
#ifndef Q_OS_MACOS
    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this, &SystemTrayNotificationHandler::onTrayActivated);
#else
    m_macStatusIcon = new MacOSStatusIcon(this);
#endif

    m_trayActionShow =  m_menu.addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME, this, [this](){
        emit raiseRequested();
    });
    m_menu.addSeparator();
    m_trayActionConnect = m_menu.addAction(tr("Connect"), this, [this](){ emit connectRequested(); });
    m_trayActionDisconnect = m_menu.addAction(tr("Disconnect"), this, [this](){ emit disconnectRequested(); });

    m_menu.addSeparator();

    m_trayActionVisitWebSite = m_menu.addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl(websiteUrl));
    });

    // Quit action: disconnect VPN first on macOS NE, else quit directly
    m_trayActionQuit = m_menu.addAction(QIcon(":/images/tray/cancel.png"),
                                       tr("Quit") + " " + APPLICATION_NAME,
                                       this,
                                       [&](){ qApp->quit(); });

#ifdef Q_OS_MACOS
    m_macStatusIcon->setMenu(m_menu.toNSMenu());
    m_macStatusIcon->setToolTip(APPLICATION_NAME);
    m_macStatusIcon->setIndicatorColor(QColor());
#else
    m_systemTrayIcon.setContextMenu(&m_menu);
#endif
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

void SystemTrayNotificationHandler::setTrayIcon(const QString &iconPath)
{
    QIcon trayIconMask(QPixmap(iconPath).scaled(128,128));
#ifndef Q_OS_MAC
    trayIconMask.setIsMask(true);
#endif
    m_systemTrayIcon.setIcon(trayIconMask);
}

#ifdef Q_OS_MACOS
void SystemTrayNotificationHandler::setMacTrayIcon(bool active)
{
    const auto icon = buildMacMenuBarVpnIcon(active);
    m_macStatusIcon->setLength(icon.itemLength);
    m_macStatusIcon->setImageSize(icon.imageWidth, icon.imageHeight);
    m_macStatusIcon->setIconData(encodePixmapAsPng(icon.pixmap), false);
}
#endif

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
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
#endif
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case Vpn::ConnectionState::Preparing:
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
#endif
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Connecting:
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
#endif
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Connected:
#ifdef Q_OS_MACOS
        setMacTrayIcon(true);
#else
        setTrayIcon(QString(resourcesPath).arg(ConnectedTrayIconName));
#endif
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Disconnecting:
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
#endif
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Reconnecting:
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
#endif
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
        break;
    case Vpn::ConnectionState::Error:
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(ErrorTrayIconName));
#endif
        m_trayActionConnect->setEnabled(true);
        m_trayActionDisconnect->setEnabled(false);
        break;
    case Vpn::ConnectionState::Unknown:
    default:
        m_trayActionConnect->setEnabled(false);
        m_trayActionDisconnect->setEnabled(true);
#ifdef Q_OS_MACOS
        setMacTrayIcon(false);
#else
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
#endif
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
#ifdef Q_OS_MACOS
  Q_UNUSED(timerMsec);
  m_macStatusIcon->showMessage(title, message);
#else
  QIcon icon(ConnectedTrayIconName);
  m_systemTrayIcon.showMessage(title, message, icon, timerMsec);
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
