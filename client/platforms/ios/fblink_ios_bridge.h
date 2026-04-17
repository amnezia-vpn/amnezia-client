#ifndef FBLINK_IOS_BRIDGE_H
#define FBLINK_IOS_BRIDGE_H

#include <string>

namespace FBLink {
std::string swiftUpdateLogData(const std::string &qtString);
void swiftDeleteLog();
void toggleLogging(bool isEnabled);
void clearSettings();
void toggleScreenshots(bool isEnabled);
void removeVPNC(const std::string &vpncName);
}

#endif // FBLINK_IOS_BRIDGE_H
