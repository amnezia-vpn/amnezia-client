#ifndef MACTRAYICONBACKEND_H
#define MACTRAYICONBACKEND_H

#include "ui/utils/trayIconBackend.h"

#include "macosstatusicon.h"

class MacTrayIconBackend final : public TrayIconBackend
{
public:
    explicit MacTrayIconBackend(QObject *parent);

    void setMenu(QMenu *menu) override;
    void setToolTip(const QString &tooltip) override;
    void show() override;
    void applyVisual(const TrayIconVisual &visual) override;
    void showMessage(const QString &title, const QString &message, const TrayIconVisual &visual, int timerMsec) override;
    void rebuildMenu() override;
    void setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler) override;

private:
    MacOSStatusIcon m_statusIcon;
};

#endif // MACTRAYICONBACKEND_H
