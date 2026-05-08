#ifndef IOS_PAIRING_CAMERA_ACCESS_H
#define IOS_PAIRING_CAMERA_ACCESS_H

#include <functional>

bool amneziaIosPairingCameraAccessGranted();
void amneziaIosRequestPairingCameraAccess(const std::function<void(bool)> &onDone);
void amneziaIosOpenApplicationSettings();

/** When true, makes Qt's root UIView non-opaque so window-layer camera preview shows through transparent QML. */
void amneziaIosApplyEmbeddedCameraUnderlayToQtView(bool enable);

/**
 * Extra height (points) added to the bottom native dim strip above UIKit safe-area bottom, when the Qt root view
 * fills the host (tab bar lives in QML). Typically set from QML scanDimBleedBottom. Pass 0 to clear.
 */
void amneziaIosSetPairingEmbeddedCameraNativeBottomExtraPt(int extraPt);

/** Call after AVCaptureVideoPreviewLayer is inserted on UIWindow so the window-layer mask stacks above it. */
void amneziaIosPairingRelayoutChromeIfNeeded(void);

#endif
