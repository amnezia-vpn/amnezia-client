# Client Stability — Design Specification

**Date**: 2026-04-28  
**Priority**: High  
**Focus**: First launch freeze + reconnection loop

---

## Problem Statement

### Issue 1: First Launch Freeze
- **Symptom**: Application freezes for 5 minutes on first launch
- **Frequency**: Only first launch, subsequent launches work normally
- **Impact**: Terrible first impression, users think app is broken

### Issue 2: Infinite Reconnection Loop
- **Symptom**: VPN attempts reconnection infinitely without success
- **Trigger**: Connection drops (network change, server unreachable)
- **Impact**: Battery drain, UI unresponsive, user frustration

---

## Root Cause Analysis

### First Launch Freeze

**Hypothesis**: Blocking initialization on main thread

Likely causes:
1. **Network requests on UI thread** (server list fetch, auth check)
2. **Heavy crypto operations** (key generation, certificate validation)
3. **Database initialization** (SQLite schema creation, migrations)
4. **File system operations** (config files, cache creation)
5. **Synchronous IPC** (waiting for service to start)

**Investigation needed**:
- Profile first launch with Qt profiler
- Add timing logs to initialization sequence
- Check which component blocks for ~5 minutes

### Infinite Reconnection Loop

**Root cause**: No backoff strategy + no failure threshold

Current behavior (assumed):
```cpp
void reconnect() {
    while (true) {
        if (connect()) break;
        // No delay, no limit → infinite loop
    }
}
```

Missing:
- Exponential backoff between attempts
- Maximum retry count
- Failure reason detection (permanent vs temporary)
- User notification after N failures

---

## Architecture

### First Launch Optimization

**Async Initialization Pipeline**:
```
App Start (UI thread)
    ↓
Show Splash Screen
    ↓
    ├─ [Background Thread 1] Load Config
    ├─ [Background Thread 2] Initialize Database
    ├─ [Background Thread 3] Fetch Server List
    ├─ [Background Thread 4] Start Service IPC
    └─ [Background Thread 5] Validate Certificates
    ↓
Wait for Critical Tasks (with timeout)
    ↓
Show Main Window
    ↓
Continue Non-Critical Init in Background
```

**Components**:

1. **Async Initializer** (`client/core/async_initializer.h`)
   - Manages parallel initialization tasks
   - Tracks dependencies between tasks
   - Provides progress feedback to UI
   - Timeout handling for stuck tasks

2. **Lazy Loading Manager** (`client/core/lazy_loader.h`)
   - Defers non-critical initialization
   - Loads on-demand when needed
   - Examples: translations, themes, analytics

3. **Startup Profiler** (`client/core/startup_profiler.h`)
   - Measures time for each init step
   - Logs slow operations (>100ms)
   - Telemetry for optimization

### Reconnection Strategy

**Smart Reconnection with Backoff**:
```
Connection Lost
    ↓
Attempt 1: Immediate (0s delay)
    ↓ Failed
Attempt 2: 2s delay
    ↓ Failed
Attempt 3: 4s delay
    ↓ Failed
Attempt 4: 8s delay
    ↓ Failed
Attempt 5: 16s delay
    ↓ Failed
Max Attempts Reached (5)
    ↓
Notify User: "Unable to reconnect. Check connection."
    ↓
Stop Auto-Reconnect
```

**Components**:

1. **Reconnection Manager** (`client/core/reconnection_manager.h`)
   - Exponential backoff algorithm
   - Max retry limit (configurable)
   - Failure reason classification
   - User notification on exhaustion

2. **Connection Health Monitor** (`client/core/connection_health.h`)
   - Detects connection quality
   - Distinguishes temporary vs permanent failures
   - Adjusts strategy based on failure type

3. **Network Change Detector** (`client/core/network_detector.h`)
   - Listens for network interface changes
   - Resets retry counter on network change
   - Triggers immediate reconnect on Wi-Fi → mobile data

---

## Data Flow

### First Launch — Before (Blocking)
```
main()
    ↓
[UI Thread] Initialize Everything (5 minutes)
    ├─ Load config (blocking file I/O)
    ├─ Init database (blocking SQLite)
    ├─ Fetch servers (blocking HTTP)
    ├─ Start service (blocking IPC)
    └─ Validate certs (blocking crypto)
    ↓
Show Window (finally!)
```

### First Launch — After (Async)
```
main()
    ↓
Show Splash (50ms)
    ↓
[Background] Parallel Init
    ├─ Thread 1: Config (100ms)
    ├─ Thread 2: Database (200ms)
    ├─ Thread 3: Servers (500ms) ← Can fail, retry later
    ├─ Thread 4: Service (300ms)
    └─ Thread 5: Certs (150ms)
    ↓
Wait for Critical (max 2s timeout)
    ↓
Show Window (total: ~2s)
    ↓
[Background] Continue non-critical init
```

### Reconnection — Before (Infinite Loop)
```
Connection Lost
    ↓
while (true) {
    connect()  // Fails immediately
    // No delay → CPU spin, battery drain
}
```

### Reconnection — After (Smart Backoff)
```
Connection Lost
    ↓
ReconnectionManager::Start()
    ↓
For attempt in 1..MAX_ATTEMPTS:
    ↓
    Wait(exponential_backoff(attempt))
    ↓
    result = connect()
    ↓
    if (result == SUCCESS):
        return SUCCESS
    ↓
    if (result == PERMANENT_FAILURE):
        break  // Don't retry auth failures
    ↓
Notify User: "Reconnection failed"
Stop
```

---

## Implementation Details

### Async Initializer Interface

```cpp
class AsyncInitializer {
public:
    struct Task {
        std::string name;
        std::function<bool()> execute;
        bool critical;  // Must complete before showing UI
        std::chrono::milliseconds timeout;
    };
    
    void AddTask(Task task);
    void Start();
    bool WaitForCritical(std::chrono::milliseconds max_wait);
    
signals:
    void progressChanged(int percent);
    void taskCompleted(QString name, bool success);
    void allCriticalCompleted();
};
```

### Reconnection Manager Interface

```cpp
class ReconnectionManager {
public:
    struct Config {
        int max_attempts = 5;
        std::chrono::seconds initial_delay = 2s;
        double backoff_multiplier = 2.0;
        std::chrono::seconds max_delay = 60s;
    };
    
    enum class FailureType {
        Temporary,   // Network unreachable, timeout
        Permanent,   // Auth failed, server removed
        Unknown
    };
    
    void Start();
    void Stop();
    void Reset();  // Called on network change
    
signals:
    void attemptStarted(int attempt_number);
    void attemptFailed(FailureType type);
    void reconnected();
    void exhausted();  // Max attempts reached
};
```

### Network Change Detector Interface

```cpp
class NetworkChangeDetector : public QObject {
public:
    enum class ChangeType {
        InterfaceUp,
        InterfaceDown,
        WiFiToMobile,
        MobileToWiFi,
        IPAddressChanged
    };
    
signals:
    void networkChanged(ChangeType type);
};
```

---

## Error Handling

### First Launch Failures

**Critical task timeout** (e.g., service won't start):
- Show error dialog with details
- Offer "Retry" or "Continue Anyway"
- Log full diagnostic info

**Non-critical task failure** (e.g., server list fetch):
- Continue with cached/default data
- Retry in background
- Show warning in UI (not blocking)

**Database corruption**:
- Backup corrupted DB
- Create fresh database
- Notify user (data loss)

### Reconnection Failures

**Temporary failure** (network unreachable):
- Continue retry with backoff
- Show "Reconnecting..." in UI

**Permanent failure** (auth expired):
- Stop retry immediately
- Show "Session expired, please login"
- Navigate to login screen

**Max attempts exhausted**:
- Show notification: "Unable to reconnect"
- Offer manual reconnect button
- Log failure reason for diagnostics

---

## Testing Strategy

### First Launch Performance Tests

**Baseline Measurement**:
1. Fresh install on clean VM
2. Measure time to main window
3. Profile with Qt Creator profiler
4. Identify bottlenecks

**After Optimization**:
1. Repeat measurement
2. Target: <2s to main window
3. Verify no blocking operations on UI thread

**Stress Tests**:
- Slow network (simulate with tc/netem)
- Service not responding
- Corrupted config files
- Missing dependencies

### Reconnection Tests

**Unit Tests** (`test_reconnection_manager.cpp`):
- Backoff calculation correctness
- Max attempts enforcement
- Failure type classification

**Integration Tests**:
1. **Temporary failure**: Disconnect network → reconnects after backoff
2. **Permanent failure**: Invalid credentials → stops immediately
3. **Network change**: Wi-Fi → mobile → resets counter, reconnects
4. **Max attempts**: 5 failures → stops, notifies user

**Manual Tests**:
1. Disconnect Wi-Fi during active VPN → observe backoff delays
2. Turn off VPN server → verify stops after max attempts
3. Switch networks → verify immediate reconnect attempt
4. Airplane mode → verify graceful handling

---

## Success Criteria

### First Launch
- ✅ Main window appears in <2 seconds
- ✅ No UI freeze (app remains responsive)
- ✅ Progress indicator shows initialization status
- ✅ Graceful handling of initialization failures

### Reconnection
- ✅ No infinite loops (max 5 attempts)
- ✅ Exponential backoff between attempts
- ✅ User notified after exhaustion
- ✅ Network change triggers immediate retry
- ✅ Battery drain eliminated

---

## Backward Compatibility

- Existing config files remain compatible
- No breaking changes to IPC protocol
- Graceful degradation if service is old version

---

## Implementation Order

1. **Startup Profiler** (measure current state)
2. **Async Initializer** (framework)
3. **Migrate blocking init to async** (one component at a time)
4. **Reconnection Manager** (backoff logic)
5. **Network Change Detector** (platform-specific)
6. **Integration** (wire everything together)
7. **Testing** (performance + reconnection scenarios)

---

## Dependencies

- Qt Concurrent (for async tasks)
- Qt Network (for network change detection)
- Existing connection management code
- IPC client interface

---

## Risks & Mitigations

**Risk**: Async init introduces race conditions  
**Mitigation**: Careful dependency management, critical path synchronization

**Risk**: Backoff too aggressive (user waits too long)  
**Mitigation**: Configurable parameters, user can trigger manual reconnect

**Risk**: Network detector false positives  
**Mitigation**: Debounce network events, verify connectivity before reconnect

---

## Future Enhancements

- Adaptive backoff based on failure history
- Predictive reconnection (anticipate network changes)
- Background connectivity checks (detect issues before user notices)
- Telemetry: track reconnection success rate
