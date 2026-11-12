#include "splitTunnelingUpdateBackend.h"

#ifdef AMNEZIA_DESKTOP
    #include "desktopSplitTunnelingUpdateBackend.h"
#elif defined(AMNEZIA_APPLE_LIVE_ROUTING)
    #include "appleSplitTunnelingUpdateBackend.h"
#endif

SplitTunnelingUpdateBackend *SplitTunnelingUpdateBackend::create(QObject *parent)
{
#ifdef AMNEZIA_DESKTOP
    return new DesktopSplitTunnelingUpdateBackend(parent);
#elif defined(AMNEZIA_APPLE_LIVE_ROUTING)
    return new AppleSplitTunnelingUpdateBackend(parent);
#else
    Q_UNUSED(parent)
    return nullptr;
#endif
}
