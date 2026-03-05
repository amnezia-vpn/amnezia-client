/* MinGW compatibility header for missing Windows SDK symbols.
 * This file provides definitions that are present in MSVC Windows SDK
 * but absent from MinGW's headers.
 *
 * IMPORTANT: Include this file AFTER all standard Windows headers.
 * Each translation unit that uses WFP GUIDs must include this file once.
 * The GUIDs are defined as static const variables to avoid ODR issues.
 */

#pragma once

#ifndef MINGW_COMPAT_H
#define MINGW_COMPAT_H

// -----------------------------------------------------------------------
// WFP (Windows Filtering Platform) missing definitions for MinGW
// -----------------------------------------------------------------------
#ifdef __MINGW32__

// FwpmTransactionBegin is an alias for FwpmTransactionBegin0 in MSVC SDK
#ifndef FwpmTransactionBegin
#define FwpmTransactionBegin FwpmTransactionBegin0
#endif

// FWPM_SESSION_FLAG_DYNAMIC
#ifndef FWPM_SESSION_FLAG_DYNAMIC
#define FWPM_SESSION_FLAG_DYNAMIC 0x00000001
#endif

// FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT
#ifndef FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT
#define FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT 0x00000001
#endif

// FWP_CONDITION_L2_IS_VM2VM flag (for Hyper-V)
#ifndef FWP_CONDITION_L2_IS_VM2VM
#define FWP_CONDITION_L2_IS_VM2VM 0x00000200
#endif

// -----------------------------------------------------------------------
// WFP GUIDs - defined as static const to avoid duplicate symbol errors.
// These GUIDs come from the Windows Filtering Platform SDK.
// -----------------------------------------------------------------------

// Helper macro: define a GUID as a static const variable (no ODR issues)
#define MINGW_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

// ALE Auth Connect / Recv Accept layers
#ifndef FWPM_LAYER_ALE_AUTH_CONNECT_V4_DEFINED
#define FWPM_LAYER_ALE_AUTH_CONNECT_V4_DEFINED
MINGW_DEFINE_GUID(FWPM_LAYER_ALE_AUTH_CONNECT_V4,
    0xc38d57d1, 0x05a7, 0x4c33, 0x90, 0x4f, 0x7f, 0xbc, 0xee, 0xe6, 0x0e, 0x82);
#endif
#ifndef FWPM_LAYER_ALE_AUTH_CONNECT_V6_DEFINED
#define FWPM_LAYER_ALE_AUTH_CONNECT_V6_DEFINED
MINGW_DEFINE_GUID(FWPM_LAYER_ALE_AUTH_CONNECT_V6,
    0x4a72393b, 0x319f, 0x44bc, 0x84, 0xc3, 0xba, 0x54, 0xdc, 0xb3, 0xb6, 0xb4);
#endif
#ifndef FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DEFINED
#define FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DEFINED
MINGW_DEFINE_GUID(FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
    0xe1cd9fe7, 0xf4b5, 0x4273, 0x96, 0xc0, 0x59, 0x2e, 0x48, 0x7b, 0x86, 0x50);
#endif
#ifndef FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DEFINED
#define FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DEFINED
MINGW_DEFINE_GUID(FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
    0xa3b3ab6b, 0x3564, 0x488c, 0x9c, 0x47, 0x8b, 0xd9, 0x14, 0xbe, 0xa4, 0xdc);
#endif

// MAC frame native layers (used for Hyper-V traffic filtering)
#ifndef FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_DEFINED
#define FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_DEFINED
MINGW_DEFINE_GUID(FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE,
    0xd4220bd3, 0x62ce, 0x4f08, 0xae, 0x88, 0xb4, 0x56, 0x0f, 0xd3, 0x56, 0xcd);
#endif
#ifndef FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_DEFINED
#define FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_DEFINED
MINGW_DEFINE_GUID(FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE,
    0xbb566f0b, 0x3a65, 0x045e, 0x9c, 0x02, 0x03, 0xc4, 0x18, 0x1e, 0xee, 0x2d);
#endif

// WFP Condition GUIDs
#ifndef FWPM_CONDITION_IP_PROTOCOL_DEFINED
#define FWPM_CONDITION_IP_PROTOCOL_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_IP_PROTOCOL,
    0x3971ef2b, 0x623e, 0x4f9a, 0x8c, 0xb1, 0x6e, 0x79, 0xb8, 0x06, 0xb9, 0xa7);
#endif
#ifndef FWPM_CONDITION_IP_LOCAL_PORT_DEFINED
#define FWPM_CONDITION_IP_LOCAL_PORT_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_IP_LOCAL_PORT,
    0x0c1ba1af, 0xeb57, 0x4cd5, 0xa5, 0x19, 0x92, 0x1a, 0xe8, 0x2f, 0xba, 0x06);
#endif
#ifndef FWPM_CONDITION_IP_REMOTE_PORT_DEFINED
#define FWPM_CONDITION_IP_REMOTE_PORT_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_IP_REMOTE_PORT,
    0xc35a604d, 0xd22b, 0x4e1a, 0x91, 0xb4, 0x68, 0xf6, 0x74, 0xee, 0x67, 0x4b);
#endif
#ifndef FWPM_CONDITION_IP_REMOTE_ADDRESS_DEFINED
#define FWPM_CONDITION_IP_REMOTE_ADDRESS_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_IP_REMOTE_ADDRESS,
    0xb235f6ea, 0x5f9f, 0x4b60, 0x9f, 0x34, 0x1c, 0x94, 0xde, 0x2b, 0x7f, 0x1c);
#endif
#ifndef FWPM_CONDITION_INTERFACE_INDEX_DEFINED
#define FWPM_CONDITION_INTERFACE_INDEX_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_INTERFACE_INDEX,
    0x5e62e224, 0x4f62, 0x4f52, 0x8a, 0x62, 0x07, 0x5f, 0x5a, 0x5e, 0xa6, 0x86);
#endif
#ifndef FWPM_CONDITION_ALE_APP_ID_DEFINED
#define FWPM_CONDITION_ALE_APP_ID_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_ALE_APP_ID,
    0xd78e1e87, 0x8644, 0x4ea5, 0x94, 0x37, 0xd8, 0x09, 0xec, 0xef, 0xc9, 0x71);
#endif
#ifndef FWPM_CONDITION_L2_FLAGS_DEFINED
#define FWPM_CONDITION_L2_FLAGS_DEFINED
MINGW_DEFINE_GUID(FWPM_CONDITION_L2_FLAGS,
    0x4df9b9d9, 0xdc2e, 0x49f0, 0x98, 0xb0, 0xde, 0xba, 0xc0, 0x52, 0x58, 0xcc);
#endif

#endif  // __MINGW32__

// -----------------------------------------------------------------------
// DNS_INTERFACE_SETTINGS - added in Windows SDK 10.0.19041 (missing in MinGW)
// -----------------------------------------------------------------------
#ifdef __MINGW32__

#ifndef DNS_INTERFACE_SETTINGS_VERSION1
#define DNS_INTERFACE_SETTINGS_VERSION1 1
#endif

#ifndef DNS_SETTING_NAMESERVER
#define DNS_SETTING_NAMESERVER    0x0008
#endif
#ifndef DNS_SETTING_SEARCHLIST
#define DNS_SETTING_SEARCHLIST    0x0010
#endif
#ifndef DNS_SETTING_IPV6
#define DNS_SETTING_IPV6          0x0100
#endif

typedef struct _DNS_INTERFACE_SETTINGS {
    ULONG    Version;
    ULONG64  Flags;
    PWSTR    Domain;
    PWSTR    NameServer;
    PWSTR    SearchList;
    ULONG    RegistrationEnabled;
    ULONG    RegisterAdapterName;
    ULONG    EnableLLMNR;
    ULONG    QueryAdapterName;
    PWSTR    ProfileNameServer;
} DNS_INTERFACE_SETTINGS;

#endif  // __MINGW32__

// -----------------------------------------------------------------------
// IKEv2 / RAS constants missing from older MinGW ras.h
// -----------------------------------------------------------------------
#ifdef __MINGW32__

#ifndef RASNP_Ipv6
#define RASNP_Ipv6  0x00000008
#endif

#ifndef RASEO2_RequireMachineCertificates
#define RASEO2_RequireMachineCertificates  0x00000040
#endif

// VPN Strategy values (RASENTRY.dwVpnStrategy)
#ifndef VS_Ikev2Only
#define VS_Ikev2Only  7
#endif
#ifndef VS_Ikev2First
#define VS_Ikev2First 8
#endif

#endif  // __MINGW32__

#endif  // MINGW_COMPAT_H
