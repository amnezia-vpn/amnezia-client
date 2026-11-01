#include "splitTunnelingUpdateBackend.h"

#ifdef AMNEZIA_DESKTOP
    #include "desktopSplitTunnelingUpdateBackend.h"
#endif

SplitTunnelingUpdateBackend *SplitTunnelingUpdateBackend::create(QObject *parent)
{
#ifdef AMNEZIA_DESKTOP
    return new DesktopSplitTunnelingUpdateBackend(parent);
#else
    Q_UNUSED(parent)
    return nullptr;
#endif
}
