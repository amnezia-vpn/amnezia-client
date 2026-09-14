#ifndef SWIFTBRIDGE_H
#define SWIFTBRIDGE_H

#if defined(Q_OS_IOS) || defined(MACOS_NE)
#include "core/utils/swiftBridgeConfig.h"

#ifndef SWIFT_BRIDGE_OBJC_HEADER
#ifdef AMNEZIA_SWIFT_OBJC_HEADER
#define SWIFT_BRIDGE_OBJC_HEADER AMNEZIA_SWIFT_OBJC_HEADER
#else
#define SWIFT_BRIDGE_OBJC_HEADER "AmneziaVPN-Swift.h"
#endif
#endif
#ifndef SWIFT_BRIDGE_NAMESPACE
#ifdef AMNEZIA_SWIFT_NAMESPACE
#define SWIFT_BRIDGE_NAMESPACE AMNEZIA_SWIFT_NAMESPACE
#else
#define SWIFT_BRIDGE_NAMESPACE AmneziaVPN
#endif
#endif

#include SWIFT_BRIDGE_OBJC_HEADER
#endif

#endif
