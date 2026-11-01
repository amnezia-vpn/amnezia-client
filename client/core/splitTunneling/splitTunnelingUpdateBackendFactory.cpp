#include "splitTunnelingUpdateBackend.h"

#ifdef AMNEZIA_DESKTOP
    #include "desktopSplitTunnelingUpdateBackend.h"
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "appleSplitTunnelingUpdateBackend.h"
#endif

SplitTunnelingUpdateBackend *SplitTunnelingUpdateBackend::create(QObject *parent)
{
#ifdef AMNEZIA_DESKTOP
    return new DesktopSplitTunnelingUpdateBackend(parent);
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
    return new AppleSplitTunnelingUpdateBackend(parent);
#else
    Q_UNUSED(parent)
    return nullptr;
#endif
}
