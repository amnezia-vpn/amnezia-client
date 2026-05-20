#pragma once

#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handles custom scheme vpn:// (full absoluteString) and file URLs for config / backup import. */
void AmneziaHandleOpenUrl(NSURL *url);

#ifdef __cplusplus
}
#endif
