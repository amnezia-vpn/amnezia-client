#ifndef LINUXTRAYICONBACKEND_H
#define LINUXTRAYICONBACKEND_H

#include "ui/utils/trayIconBackend.h"

#include <QColor>
#include <QIcon>
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
    QIcon buildTrayIcon(qreal opacity, bool darkTheme, const QColor &indicatorColor) const;

    QSystemTrayIcon m_trayIcon;
};

#endif // LINUXTRAYICONBACKEND_H
