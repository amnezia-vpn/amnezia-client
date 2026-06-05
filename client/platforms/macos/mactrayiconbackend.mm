#include "mactrayiconbackend.h"

#include "ui/utils/trayIconCommon.h"

MacTrayIconBackend::MacTrayIconBackend(QObject *parent)
    : m_statusIcon(parent)
{
}

void MacTrayIconBackend::setMenu(QMenu *menu)
{
    m_statusIcon.setMenu(menu);
}

void MacTrayIconBackend::setToolTip(const QString &tooltip)
{
    m_statusIcon.setToolTip(tooltip);
}

void MacTrayIconBackend::show()
{
}

void MacTrayIconBackend::applyVisual(const TrayIconVisual &visual)
{
    m_statusIcon.setIconFromData(TrayIconCommon::buildTemplatePng(visual.connectionState));
}

void MacTrayIconBackend::showMessage(const QString &title, const QString &message, const TrayIconVisual &visual, int timerMsec)
{
    Q_UNUSED(visual);
    Q_UNUSED(timerMsec);
    m_statusIcon.showMessage(title, message);
}

void MacTrayIconBackend::rebuildMenu()
{
    m_statusIcon.rebuildNativeMenu();
}

void MacTrayIconBackend::setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler)
{
    Q_UNUSED(handler);
}

std::unique_ptr<TrayIconBackend> createTrayIconBackend(QObject *parent)
{
    return std::make_unique<MacTrayIconBackend>(parent);
}
