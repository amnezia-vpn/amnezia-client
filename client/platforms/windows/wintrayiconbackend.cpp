#include "wintrayiconbackend.h"

#include "ui/utils/trayIconCommon.h"

#include <QObject>

WinTrayIconBackend::WinTrayIconBackend(QObject *parent) : m_trayIcon(parent)
{
}

void WinTrayIconBackend::setMenu(QMenu *menu)
{
    m_trayIcon.setContextMenu(menu);
}

void WinTrayIconBackend::setToolTip(const QString &tooltip)
{
    m_trayIcon.setToolTip(tooltip);
}

void WinTrayIconBackend::show()
{
    m_trayIcon.show();
}

void WinTrayIconBackend::applyVisual(const TrayIconVisual &visual)
{
    const qreal opacity = TrayIconCommon::opacityForState(visual.connectionState);
    const QColor indicatorColor = TrayIconCommon::indicatorColorForState(visual.connectionState);
    m_trayIcon.setIcon(buildTrayIcon(opacity, visual.darkTheme, indicatorColor));
}

void WinTrayIconBackend::showMessage(const QString &title, const QString &message, const TrayIconVisual &visual,
                                     int timerMsec)
{
    m_trayIcon.showMessage(title, message,
                           buildTrayIcon(TrayIconCommon::kConnectedOpacity, visual.darkTheme,
                                         TrayIconCommon::indicatorColorForState(Vpn::ConnectionState::Connected)),
                           timerMsec);
}

void WinTrayIconBackend::rebuildMenu()
{
}

void WinTrayIconBackend::setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler)
{
    if (!handler) {
        return;
    }

    QObject::connect(&m_trayIcon, &QSystemTrayIcon::activated, m_trayIcon.parent(),
                     [handler](QSystemTrayIcon::ActivationReason reason) { handler(reason); });
}

QIcon WinTrayIconBackend::buildTrayIcon(qreal opacity, bool darkTheme, const QColor &indicatorColor) const
{
    return TrayIconCommon::buildIcon(opacity, darkTheme, indicatorColor);
}

std::unique_ptr<TrayIconBackend> createTrayIconBackend(QObject *parent)
{
    return std::make_unique<WinTrayIconBackend>(parent);
}
