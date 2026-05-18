#ifndef IOS_PAIRING_QR_OVERLAY_WINDOW_H
#define IOS_PAIRING_QR_OVERLAY_WINDOW_H

#include <functional>
#include <string>

using AmneziaPairingQrScannedUtf8Handler = std::function<void(const char *)>;
using AmneziaPairingQrOverlayBackHandler = std::function<void()>;

void amneziaIosPairingQrOverlayPresent(AmneziaPairingQrScannedUtf8Handler onScanned, AmneziaPairingQrOverlayBackHandler onBack,
                                       const std::string &titleUtf8, const std::string &subtitleUtf8);
void amneziaIosPairingQrOverlayDismiss();
bool amneziaIosPairingQrOverlayIsPresented();
void amneziaIosPairingQrOverlaySetTorchEnabled(bool on);
void amneziaIosPairingQrOverlayRestartCapture();

#endif
