#include "platforms/ios/iosPairingCameraAccess.h"

bool amneziaIosPairingCameraAccessGranted()
{
    return true;
}

void amneziaIosRequestPairingCameraAccess(const std::function<void(bool)> &onDone)
{
    onDone(true);
}

void amneziaIosOpenApplicationSettings() {}

void amneziaIosApplyEmbeddedCameraUnderlayToQtView(bool) {}

void amneziaIosSetPairingEmbeddedCameraNativeBottomExtraPt(int) {}

void amneziaIosPairingRelayoutChromeIfNeeded(void) {}
