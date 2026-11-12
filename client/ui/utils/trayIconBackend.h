#ifndef TRAYICONBACKEND_H
#define TRAYICONBACKEND_H

#include <functional>
#include <memory>

#include <QMenu>
#include <QString>
#include <QSystemTrayIcon>

#include "core/protocols/vpnProtocol.h"

class QObject;

struct TrayIconVisual {
    Vpn::ConnectionState connectionState = Vpn::ConnectionState::Unknown;
    bool darkTheme = false;
};

class TrayIconBackend {
public:
    virtual ~TrayIconBackend() = default;

    virtual void setMenu(QMenu *menu) = 0;
    virtual void setToolTip(const QString &tooltip) = 0;
    virtual void show() = 0;
    virtual void applyVisual(const TrayIconVisual &visual) = 0;
    virtual void showMessage(const QString &title, const QString &message, const TrayIconVisual &visual, int timerMsec) = 0;
    virtual void rebuildMenu() = 0;
    virtual void setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler) = 0;
};

std::unique_ptr<TrayIconBackend> createTrayIconBackend(QObject *parent);

#endif // TRAYICONBACKEND_H
