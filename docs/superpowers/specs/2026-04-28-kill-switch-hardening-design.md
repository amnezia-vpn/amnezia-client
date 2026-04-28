# Kill Switch Hardening — Design Specification

**Date**: 2026-04-28  
**Priority**: Critical  
**Platforms**: Windows, Linux, macOS, Android, iOS

---

## Problem Statement

After system reboot with VPN enabled:
- VPN is off, but kill switch firewall rules remain active
- Attempting to connect VPN breaks entire network
- Only solution: reboot with VPN disabled
- **Reproduces on all platforms**

### Root Cause
System service (daemon) does not clean up stale firewall state on startup when:
1. VPN was active before reboot
2. Client did not properly deactivate kill switch
3. Firewall rules remain in system

---

## Architecture

### Startup State Recovery Flow
```
System Boot
    ↓
Service Start
    ↓
Check Persisted State (file/registry)
    ↓
    ├─ VPN was active? → Restore connection OR cleanup
    ├─ Kill switch was active? → Verify + cleanup if VPN off
    └─ Stale rules detected? → Force cleanup
    ↓
Ready for Client Connection
```

### Components

#### 1. State Persistence Layer
**Location**: `service/state_manager.h`, `service/state_manager.cpp`

**Responsibilities**:
- Save VPN/kill switch state before shutdown
- Read state on startup
- Platform-independent interface

**Interface**:
```cpp
class StateManager {
public:
    struct ServiceState {
        bool vpn_active;
        bool killswitch_active;
        std::string protocol;
        std::string server_id;
        std::vector<std::string> applied_rules;
        std::chrono::system_clock::time_point timestamp;
    };
    
    static bool SaveState(const ServiceState& state);
    static std::optional<ServiceState> LoadState();
    static bool ClearState();
};
```

#### 2. Firewall State Validator
**Location**: `service/firewall_validator.h`, `service/firewall_validator.cpp`

**Responsibilities**:
- Check current firewall rules
- Detect stale/orphaned rules
- Compare with expected state

**Interface**:
```cpp
class FirewallValidator {
public:
    struct FirewallState {
        std::vector<std::string> active_rules;
        bool has_killswitch_rules;
    };
    
    static FirewallState GetCurrentState();
    static bool HasStaleRules(const FirewallState& current, 
                              const StateManager::ServiceState& expected);
};
```

#### 3. Startup Recovery Handler
**Location**: `service/startup_recovery.h`, `service/startup_recovery.cpp`

**Responsibilities**:
- Run on service start
- Analyze persisted state + firewall state
- Decide: restore, cleanup, or force-reset

**Interface**:
```cpp
class StartupRecovery {
public:
    enum class Action {
        NoAction,           // Clean state
        RestoreConnection,  // Resume VPN
        CleanupRules,       // Remove stale rules
        ForceReset          // Nuclear option
    };
    
    static Action DetermineAction(
        const std::optional<StateManager::ServiceState>& persisted,
        const FirewallValidator::FirewallState& firewall
    );
    
    static bool ExecuteRecovery(Action action);
};
```

#### 4. Platform-Specific Cleanup
**Location**: `service/platforms/{windows,linux,macos,android,ios}/firewall_cleanup.*`

**Responsibilities**:
- Windows: netsh, WFP rules
- Linux: iptables/nftables
- macOS: pfctl
- Android/iOS: VPNService/NEPacketTunnelProvider state

**Interface**:
```cpp
class PlatformFirewallCleanup {
public:
    static bool RemoveAllKillSwitchRules();
    static bool RemoveSpecificRules(const std::vector<std::string>& rule_ids);
    static std::vector<std::string> ListKillSwitchRules();
};
```

---

## Data Flow

### Shutdown Sequence (Normal)
```
User disconnects VPN
    ↓
Client → Service: Deactivate Kill Switch
    ↓
Service: Remove firewall rules
    ↓
Service: Persist state {vpn: off, killswitch: off}
    ↓
System Shutdown
```

### Shutdown Sequence (Abnormal — crash/force shutdown)
```
VPN Active + Kill Switch Active
    ↓
System Force Shutdown (no cleanup)
    ↓
Firewall rules remain in system
    ↓
State file: {vpn: on, killswitch: on} OR missing
```

### Startup Recovery Decision Matrix

| Persisted State | Firewall Rules | Action |
|----------------|----------------|--------|
| vpn=on, ks=on | Present | Attempt reconnect OR cleanup (user pref) |
| vpn=off, ks=off | Present | **STALE STATE** → Force cleanup |
| Missing | Present | **ORPHANED RULES** → Force cleanup |
| Any | None | Clean state, no action |

**Legend**: ks = killswitch

---

## Error Handling

### Cleanup Failures
1. **Retry with escalating privileges**
   - First attempt: normal service privileges
   - Second attempt: elevated/root privileges
   - Third attempt: platform-specific fallback

2. **Logging**
   - Log which rules failed to remove
   - Log platform-specific error codes
   - Include firewall state snapshot

3. **Fallback: Nuclear Option**
   - Reset ALL VPN-related firewall rules
   - Not just kill switch rules
   - Last resort to unblock user

4. **User Notification**
   - If all attempts fail, notify user
   - Provide manual cleanup instructions
   - Log file location for support

### State File Corruption
- Treat as "missing state"
- Force cleanup firewall rules
- Recreate clean state file
- Log corruption details

### Platform-Specific Failures

**Windows**:
- WFP driver issues → fallback to `netsh`
- Permission denied → request UAC elevation
- Service not started → queue cleanup for next start

**Linux**:
- iptables locked → wait + retry (max 30s timeout)
- nftables not available → fallback to iptables
- Permission denied → log error, require manual intervention

**macOS**:
- pfctl permission denied → request user authentication
- Anchor not found → skip (already clean)
- pfctl busy → retry with backoff

**Android/iOS**:
- VPNService state inconsistent → force stop + restart
- Permission revoked → notify user to re-grant
- System VPN profile conflict → log + notify user

---

## State Persistence

### State File Location
- **Windows**: `%APPDATA%\FBLinkVPN\service_state.json`
- **Linux**: `/var/lib/fblinkvpn/service_state.json`
- **macOS**: `/Library/Application Support/FBLinkVPN/service_state.json`
- **Android**: `Context.getFilesDir()/service_state.json`
- **iOS**: App-specific storage via `FileManager`

### State File Format
```json
{
  "version": 1,
  "timestamp": "2026-04-28T11:10:45Z",
  "vpn": {
    "active": false,
    "protocol": "wireguard",
    "server_id": "server-123"
  },
  "killswitch": {
    "active": false,
    "rules_applied": [
      "FBLinkVPN_KillSwitch_IPv4_Block_All",
      "FBLinkVPN_KillSwitch_IPv6_Block_All"
    ]
  }
}
```

### Atomic Writes
- Write to temporary file first
- Verify JSON validity
- Atomic rename to final location
- Prevents corruption on crash during write

---

## Firewall Rules Tagging

All kill switch rules MUST have unique identifiers for detection:

**Windows WFP**:
- Filter name prefix: `FBLinkVPN_KillSwitch_`
- Example: `FBLinkVPN_KillSwitch_IPv4_Block_All`

**Linux iptables**:
- Comment: `--comment "FBLinkVPN-KillSwitch"`
- Chain: `FBLINKVPN_KILLSWITCH`

**Linux nftables**:
- Table: `fblinkvpn_killswitch`
- Comment metadata

**macOS pfctl**:
- Anchor: `fblinkvpn_killswitch`
- All rules under this anchor

**Android**:
- VpnService.Builder rules tagged with app package

**iOS**:
- NEPacketTunnelProvider rules in app group

---

## Testing Strategy

### Unit Tests

**State Persistence** (`test_state_manager.cpp`):
- Save/load state on all platforms
- Handle missing files
- Handle corrupted JSON
- Atomic write verification

**Firewall Validator** (`test_firewall_validator.cpp`):
- Detect stale rules (mock firewall state)
- Parse platform-specific rule formats
- Handle empty/invalid rule lists

**Decision Matrix** (`test_startup_recovery.cpp`):
- All combinations: (state × rules) → correct action
- Edge cases: missing state, corrupted state, no rules

### Integration Tests

**Startup Recovery** (`test_integration_recovery.cpp`):
1. Clean shutdown → clean startup (no action)
2. Crash with active VPN → cleanup rules
3. Missing state file → cleanup rules
4. Corrupted state → fallback + cleanup

### Platform-Specific Tests

**Windows** (`test_windows_firewall.cpp`):
- WFP rules cleanup
- netsh fallback
- UAC elevation handling

**Linux** (`test_linux_firewall.cpp`):
- iptables cleanup
- nftables cleanup
- iptables lock handling

**macOS** (`test_macos_firewall.cpp`):
- pfctl anchor cleanup
- User authentication prompt
- pfctl busy retry

**Android** (`FirewallCleanupTest.java`):
- VpnService state reset
- Permission handling

**iOS** (`FirewallCleanupTests.swift`):
- NEPacketTunnelProvider state
- Keychain cleanup

### Manual Testing Scenarios

**Scenario 1: Normal Flow**
1. Connect VPN
2. Enable kill switch
3. Disconnect VPN
4. Reboot system
5. ✅ Verify: Internet works, no stale rules

**Scenario 2: Crash Recovery**
1. Connect VPN
2. Enable kill switch
3. Force kill app (Task Manager / kill -9)
4. Reboot system
5. ✅ Verify: Stale rules cleaned up, internet works

**Scenario 3: Network Test**
1. After cleanup from Scenario 2
2. Open browser, test connectivity
3. ✅ Verify: All websites load normally

**Scenario 4: Reconnect Test**
1. After cleanup from Scenario 2
2. Launch app, connect VPN
3. ✅ Verify: VPN connects without errors
4. ✅ Verify: Kill switch works correctly

**Scenario 5: Multiple Reboots**
1. Connect VPN + kill switch
2. Reboot (without disconnect)
3. Reboot again (without starting app)
4. Start app
5. ✅ Verify: No accumulated stale rules

---

## Success Criteria

- ✅ After reboot with VPN off → network works normally
- ✅ No stale firewall rules remain
- ✅ VPN connects without issues after cleanup
- ✅ Works on all platforms (Windows/Linux/macOS/Android/iOS)
- ✅ No user intervention required for cleanup
- ✅ Logs provide clear diagnostics if issues occur

---

## Backward Compatibility

### Migration from Old Versions
- Old versions without state file → treat as "missing state"
- First startup of new version → force cleanup
- Create state file for future runs

### Existing Installations
- Detect old firewall rules (without tags)
- Clean up during first startup
- Apply new tagging scheme

---

## Implementation Order

1. **State Persistence Layer** (cross-platform)
2. **Firewall Validator** (cross-platform interface)
3. **Platform-Specific Cleanup** (one platform at a time)
4. **Startup Recovery Handler** (orchestration)
5. **Integration with Service Startup** (hook into main)
6. **Testing** (unit → integration → manual)
7. **Documentation** (user-facing + developer)

---

## Dependencies

- Existing firewall management code in `service/`
- Platform-specific privilege escalation mechanisms
- IPC communication with client (for user notifications)
- Logging infrastructure

---

## Risks & Mitigations

**Risk**: Cleanup fails, user still blocked  
**Mitigation**: Nuclear option fallback + manual instructions

**Risk**: State file corruption  
**Mitigation**: Atomic writes + treat corruption as missing state

**Risk**: Platform-specific edge cases  
**Mitigation**: Extensive platform testing + fallback mechanisms

**Risk**: Performance impact on startup  
**Mitigation**: Async recovery (don't block service start)

---

## Future Enhancements

- User preference: auto-reconnect vs. cleanup on startup
- Telemetry: track cleanup success rate
- Remote diagnostics: upload state on failure
- Self-healing: periodic background checks for stale rules
