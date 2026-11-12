#ifndef LINUXTRAYICONBACKEND_H
#define LINUXTRAYICONBACKEND_H

#include "ui/utils/trayIconBackend.h"

#include <QColor>
#include <QIcon>
#include <QString>
#include <QSystemTrayIcon>

class LinuxTrayIconBackend final : public TrayIconBackend
{
public:
    explicit LinuxTrayIconBackend(QObject *parent);

    void setMenu(QMenu *menu) override;
    void setToolTip(const QString &tooltip) override;
    void show() override;
    void applyVisual(const TrayIconVisual &visual) override;
    void showMessage(const QString &title, const QString &message, const TrayIconVisual &visual, int timerMsec) override;
    void rebuildMenu() override;
    void setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler) override;

private:
    QIcon buildTrayIcon(Vpn::ConnectionState state, bool darkTheme) const;

    QSystemTrayIcon m_trayIcon;
    Vpn::ConnectionState m_lastState = Vpn::ConnectionState::Unknown;
    bool m_lastDarkTheme = false;
    bool m_hasLastVisual = false;
};

#endif // LINUXTRAYICONBACKEND_H
