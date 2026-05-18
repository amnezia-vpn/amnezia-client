#ifndef IOS_PAIRING_CAMERA_ACCESS_H
#define IOS_PAIRING_CAMERA_ACCESS_H

#include <functional>

bool amneziaIosPairingCameraAccessGranted();
void amneziaIosRequestPairingCameraAccess(const std::function<void(bool)> &onDone);
void amneziaIosOpenApplicationSettings();

void amneziaIosApplyEmbeddedCameraUnderlayToQtView(bool enable);

void amneziaIosSetPairingEmbeddedCameraNativeBottomExtraPt(int extraPt);

void amneziaIosPairingRelayoutChromeIfNeeded(void);

#endif
