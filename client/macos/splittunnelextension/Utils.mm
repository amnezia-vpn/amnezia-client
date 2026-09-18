#import "Utils.h"

#import <Security/Security.h>
#import <arpa/inet.h>
#import <errno.h>
#import <fcntl.h>
#import <os/log.h>
#import <stdarg.h>
#import <stdatomic.h>
#import <string.h>
#import <sys/stat.h>
#import <unistd.h>

namespace {
/*! Roll the file over once it passes this size; one previous file is kept. */
const off_t kMaxLogFileSize = 32 * 1024 * 1024;

/*! Keep at most this many files from previous launches, newest first. */
const NSUInteger kMaxLogFiles = 10;

const char *LevelName(STLogLevel level)
{
    switch (level) {
    case STLogLevelDebug: return "DEBUG";
    case STLogLevelInfo: return "INFO";
    case STLogLevelError:
    default: return "ERROR";
    }
}

os_log_type_t OsLogType(STLogLevel level)
{
    switch (level) {
    case STLogLevelDebug: return OS_LOG_TYPE_DEBUG;
    case STLogLevelInfo: return OS_LOG_TYPE_DEFAULT;
    case STLogLevelError:
    default: return OS_LOG_TYPE_ERROR;
    }
}

/*! os_log accepts "%{public}@"; NSString formatting does not. Drop the
 *  "{...}" modifier that follows a '%' so the same call sites can feed both. */
NSString *StripOsLogModifiers(const char *fmt)
{
    NSMutableString *out = [NSMutableString stringWithCapacity:strlen(fmt)];
    for (const char *p = fmt; *p != '\0'; ++p) {
        if (*p == '%' && *(p + 1) == '{') {
            const char *close = strchr(p + 1, '}');
            if (close != NULL) {
                [out appendString:@"%"];
                p = close;
                continue;
            }
        }
        [out appendFormat:@"%c", *p];
    }
    return out;
}

/*! The wall clock as the log renders it. Taken when the line is produced, not
 *  when it reaches the file, so buffered lines keep their real time. */
NSString *FormatTimestamp(NSDate *when)
{
    static NSDateFormatter *formatter;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        formatter = [[NSDateFormatter alloc] init];
        // en_US_POSIX or the user's 12-hour locale turns HH into "2:16 PM".
        formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
        formatter.dateFormat = @"yyyy-MM-dd HH:mm:ss.SSS";
        formatter.timeZone = [NSTimeZone localTimeZone];
    });
    return [formatter stringFromDate:when];
}

/*! One timestamp per process, so every launch gets its own file and rotation
 *  keeps writing to the file this run started with. Shape matches the app and
 *  the service logs: AmneziaVPNSplitTunnel_2026-09-15_15-01-26.534.log */
NSString *LaunchStamp(void)
{
    static NSString *stamp;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        // en_US_POSIX, otherwise a 12-hour locale turns HH into "3 PM".
        formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
        formatter.dateFormat = @"yyyy-MM-dd_HH-mm-ss.SSS";
        formatter.timeZone = [NSTimeZone localTimeZone];
        stamp = [formatter stringFromDate:[NSDate date]];
    });
    return stamp;
}

/*! A file per launch would grow without bound; keep the newest ones and drop
 *  the rest. Rotated ".log.1" files sort next to their parent and are counted
 *  with it, which is fine - they age out together. */
void PruneOldLogs(NSString *dir, NSString *keepPath)
{
    NSFileManager *fm = [NSFileManager defaultManager];
    NSArray<NSString *> *names = [fm contentsOfDirectoryAtPath:dir error:nil];
    if (names == nil) {
        return;
    }

    NSMutableArray<NSString *> *logs = [NSMutableArray array];
    for (NSString *name in names) {
        if (![name hasPrefix:@"AmneziaVPNSplitTunnel_"]) {
            continue;
        }
        NSString *path = [dir stringByAppendingPathComponent:name];
        if (![path isEqualToString:keepPath]) {
            [logs addObject:path];
        }
    }
    if (logs.count <= kMaxLogFiles) {
        return;
    }

    [logs sortUsingSelector:@selector(compare:)];
    const NSUInteger excess = logs.count - kMaxLogFiles;
    for (NSUInteger i = 0; i < excess; ++i) {
        [fm removeItemAtPath:logs[i] error:nil];
    }
    os_log(STUtils.log, "file log: pruned %{public}lu old file(s) in %{public}@",
           (unsigned long)excess, dir);
}
} // namespace

#pragma mark - STFileLog

@implementation STFileLog

static dispatch_queue_t FileQueue(void)
{
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        queue = dispatch_queue_create("org.amnezia.split-tunnel.log", DISPATCH_QUEUE_SERIAL);
    });
    return queue;
}

/*! Owned by FileQueue(). */
static int g_fd = -1;
static NSString *g_path = nil;
static NSString *g_directory = nil;
static _Atomic bool g_debugEnabled = false;
static _Atomic bool g_fileOpen = false;

/*! Everything the extension logs before the host hands it a directory - the
 *  pid/uid/gid line and the result of joining group amnvpn among it - would
 *  otherwise reach os_log only, which is exactly the part of the startup that
 *  has to be readable in the file. Hold those lines until the file opens.
 *  Debug lines are not buffered: they arrive in the thousands and would push
 *  the interesting ones out of a bounded buffer. */
static NSMutableArray<NSString *> *g_pending = nil;
static NSUInteger g_pendingDropped = 0;
static _Atomic bool g_buffering = true;
static const NSUInteger kMaxPendingLines = 512;

+ (void)setDebugEnabled:(BOOL)enabled
{
    atomic_store_explicit(&g_debugEnabled, enabled ? true : false, memory_order_relaxed);
}

+ (BOOL)isDebugEnabled
{
    return atomic_load_explicit(&g_debugEnabled, memory_order_relaxed) ? YES : NO;
}

+ (BOOL)acceptsLevel:(STLogLevel)level
{
    if (!atomic_load_explicit(&g_fileOpen, memory_order_relaxed)) {
        // Not open yet: keep the non-debug startup lines for the flush.
        return atomic_load_explicit(&g_buffering, memory_order_relaxed)
               && level != STLogLevelDebug;
    }
    if (level == STLogLevelDebug) {
        return [self isDebugEnabled];
    }
    return YES;
}

+ (NSString *)currentPath
{
    __block NSString *path = nil;
    dispatch_sync(FileQueue(), ^{
        path = g_path;
    });
    return path;
}

+ (void)openLocked
{
    if (g_fd >= 0 || g_directory.length == 0) {
        return;
    }

    // Matches the layout the app and the service already use: a per-process
    // subfolder, with the _root suffix because the extension runs as root.
    NSString *dir = [g_directory stringByAppendingPathComponent:@"AmneziaVPNSplitTunnel_root"];
    NSError *error = nil;
    if (![[NSFileManager defaultManager] createDirectoryAtPath:dir
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:&error]) {
        os_log_error(STUtils.log, "file log: cannot create %{public}@: %{public}@", dir, error);
        return;
    }

    NSString *name = [NSString stringWithFormat:@"AmneziaVPNSplitTunnel_%@.log", LaunchStamp()];
    NSString *path = [dir stringByAppendingPathComponent:name];
    const int fd = open(path.fileSystemRepresentation, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        os_log_error(STUtils.log, "file log: cannot open %{public}@: errno=%{public}d %{public}s",
                     path, errno, strerror(errno));
        return;
    }

    g_fd = fd;
    g_path = path;
    atomic_store_explicit(&g_fileOpen, true, memory_order_relaxed);
    atomic_store_explicit(&g_buffering, false, memory_order_relaxed);
    os_log(STUtils.log, "file log: writing to %{public}@", path);
    PruneOldLogs(dir, path);

    if (g_pendingDropped > 0) {
        [self writeLineLocked:[NSString stringWithFormat:@"%@ [ERROR] file log: %lu buffered "
                                                         @"line(s) dropped before the file opened\n",
                                                         FormatTimestamp([NSDate date]),
                                                         (unsigned long)g_pendingDropped]];
        g_pendingDropped = 0;
    }
    for (NSString *line in g_pending) {
        [self writeLineLocked:line];
    }
    [g_pending removeAllObjects];
}

+ (void)rotateIfNeededLocked
{
    if (g_fd < 0) {
        return;
    }
    struct stat st;
    if (fstat(g_fd, &st) != 0 || st.st_size < kMaxLogFileSize) {
        return;
    }

    NSString *previous = [g_path stringByAppendingPathExtension:@"1"];
    [[NSFileManager defaultManager] removeItemAtPath:previous error:nil];
    [[NSFileManager defaultManager] moveItemAtPath:g_path toPath:previous error:nil];

    close(g_fd);
    g_fd = -1;
    atomic_store_explicit(&g_fileOpen, false, memory_order_relaxed);
    [self openLocked];
}

+ (void)configureWithDirectory:(NSString *)directory
{
    if (directory.length == 0) {
        os_log_error(STUtils.log, "file log: empty directory, file logging stays off");
        return;
    }
    dispatch_async(FileQueue(), ^{
        if ([g_directory isEqualToString:directory] && g_fd >= 0) {
            return;
        }
        if (g_fd >= 0) {
            close(g_fd);
            g_fd = -1;
            atomic_store_explicit(&g_fileOpen, false, memory_order_relaxed);
        }
        g_directory = [directory copy];
        [self openLocked];
    });
}

+ (void)writeLineLocked:(NSString *)line
{
    NSData *data = [line dataUsingEncoding:NSUTF8StringEncoding];
    const char *bytes = (const char *)data.bytes;
    size_t remaining = data.length;
    while (remaining > 0) {
        const ssize_t written = write(g_fd, bytes, remaining);
        if (written <= 0) {
            if (errno == EINTR) {
                continue;
            }
            os_log_error(STUtils.log, "file log: write failed errno=%{public}d", errno);
            break;
        }
        bytes += written;
        remaining -= (size_t)written;
    }
}

+ (void)appendLevel:(STLogLevel)level message:(NSString *)message
{
    NSString *copy = [message copy];
    NSDate *when = [NSDate date];
    dispatch_async(FileQueue(), ^{
        NSString *line = [NSString stringWithFormat:@"%@ [%s] %@\n",
                                                    FormatTimestamp(when), LevelName(level), copy];
        if (g_fd < 0) {
            if (!atomic_load_explicit(&g_buffering, memory_order_relaxed)) {
                return;
            }
            if (g_pending == nil) {
                g_pending = [NSMutableArray arrayWithCapacity:kMaxPendingLines];
            }
            if (g_pending.count >= kMaxPendingLines) {
                [g_pending removeObjectAtIndex:0];
                ++g_pendingDropped;
            }
            [g_pending addObject:line];
            return;
        }
        [self rotateIfNeededLocked];
        if (g_fd < 0) {
            return;
        }
        [self writeLineLocked:line];
    });
}

@end

#pragma mark - STLogWrite

void STLogWrite(STLogLevel level, const char *fmt, ...)
{
    const os_log_type_t type = OsLogType(level);
    const BOOL toOsLog = os_log_type_enabled(STUtils.log, type);
    const BOOL toFile = [STFileLog acceptsLevel:level];
    if (!toOsLog && !toFile) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    NSString *message = [[NSString alloc] initWithFormat:StripOsLogModifiers(fmt) arguments:args];
    va_end(args);

    if (toOsLog) {
        os_log_with_type(STUtils.log, type, "%{public}@", message);
    }
    if (toFile) {
        [STFileLog appendLevel:level message:message];
    }
}

#pragma mark - STStats

@implementation STStats

static _Atomic uint64_t g_tcpOpened = 0;
static _Atomic uint64_t g_tcpClosed = 0;
static _Atomic uint64_t g_udpOpened = 0;
static _Atomic uint64_t g_udpClosed = 0;
static _Atomic uint64_t g_bytesOut = 0;
static _Atomic uint64_t g_bytesIn = 0;
static _Atomic uint64_t g_toTunnel = 0;

+ (void)tcpOpened { atomic_fetch_add_explicit(&g_tcpOpened, 1, memory_order_relaxed); }
+ (void)udpOpened { atomic_fetch_add_explicit(&g_udpOpened, 1, memory_order_relaxed); }
+ (void)flowSentToTunnel { atomic_fetch_add_explicit(&g_toTunnel, 1, memory_order_relaxed); }

+ (void)tcpClosedWithOut:(uint64_t)out in:(uint64_t)in
{
    atomic_fetch_add_explicit(&g_tcpClosed, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_bytesOut, out, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_bytesIn, in, memory_order_relaxed);
}

+ (void)udpClosedWithOut:(uint64_t)out in:(uint64_t)in
{
    atomic_fetch_add_explicit(&g_udpClosed, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_bytesOut, out, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_bytesIn, in, memory_order_relaxed);
}

+ (NSString *)summary
{
    const uint64_t tcpOpened = atomic_load_explicit(&g_tcpOpened, memory_order_relaxed);
    const uint64_t tcpClosed = atomic_load_explicit(&g_tcpClosed, memory_order_relaxed);
    const uint64_t udpOpened = atomic_load_explicit(&g_udpOpened, memory_order_relaxed);
    const uint64_t udpClosed = atomic_load_explicit(&g_udpClosed, memory_order_relaxed);
    return [NSString stringWithFormat:
            @"tcp active=%llu total=%llu | udp active=%llu total=%llu | left to the tunnel=%llu | "
            @"relayed out=%llu KiB in=%llu KiB",
            tcpOpened - tcpClosed, tcpOpened,
            udpOpened - udpClosed, udpOpened,
            atomic_load_explicit(&g_toTunnel, memory_order_relaxed),
            atomic_load_explicit(&g_bytesOut, memory_order_relaxed) / 1024,
            atomic_load_explicit(&g_bytesIn, memory_order_relaxed) / 1024];
}

@end

#pragma mark - STUtils

@implementation STUtils

+ (os_log_t)log
{
    static os_log_t logger;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
#ifdef CLIENT_MACOS_ST_BUNDLE_ID
        logger = os_log_create(CLIENT_MACOS_ST_BUNDLE_ID, "proxy");
#else
        logger = os_log_create("org.amnezia.AmneziaVPN.network-extension", "proxy");
#endif
    });
    return logger;
}

+ (dispatch_queue_t)stateQueue
{
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        queue = dispatch_queue_create("org.amnezia.split-tunnel.state", DISPATCH_QUEUE_SERIAL);
    });
    return queue;
}

+ (uint64_t)nextFlowId
{
    static _Atomic uint64_t counter = 0;
    return atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed) + 1;
}

+ (NSData *)dataFromDispatchData:(dispatch_data_t)content
{
    if (content == NULL) {
        return nil;
    }
    NSMutableData *data = [NSMutableData data];
    dispatch_data_apply(content, ^bool(dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
        (void)region;
        (void)offset;
        [data appendBytes:buffer length:size];
        return true;
    });
    return data;
}

+ (NSString *)pathFromAuditTokenData:(NSData *)tokenData
{
    if (tokenData.length != sizeof(audit_token_t)) {
        STLogDebug("pathFromAuditToken: unexpected token length %{public}lu (want %{public}lu)",
                   (unsigned long)tokenData.length, (unsigned long)sizeof(audit_token_t));
        return nil;
    }

    NSDictionary *attributes = @{ (__bridge NSString *)kSecGuestAttributeAudit : tokenData };
    SecCodeRef code = NULL;
    OSStatus status = SecCodeCopyGuestWithAttributes(NULL, (__bridge CFDictionaryRef)attributes, kSecCSDefaultFlags, &code);
    if (status != errSecSuccess || code == NULL) {
        STLogDebug("pathFromAuditToken SecCodeCopyGuest status=%{public}d", (int)status);
        return nil;
    }

    CFURLRef url = NULL;
    status = SecCodeCopyPath(code, kSecCSDefaultFlags, &url);
    CFRelease(code);
    if (status != errSecSuccess || url == NULL) {
        STLogDebug("pathFromAuditToken SecCodeCopyPath status=%{public}d", (int)status);
        return nil;
    }

    NSString *path = [(__bridge NSURL *)url path];
    CFRelease(url);
    return path;
}

+ (nw_endpoint_t)copyEndpointFromHost:(NSString *)host port:(NSString *)port
{
    if (host.length == 0 || port.length == 0) {
        STLogError("copyEndpointFromHost: empty host=%{public}@ port=%{public}@", host ?: @"", port ?: @"");
        return NULL;
    }
    return nw_endpoint_create_host(host.UTF8String, port.UTF8String);
}

+ (BOOL)isIPv4Address:(NSString *)value
{
    if (value.length == 0) {
        return NO;
    }
    struct in_addr addr;
    return inet_pton(AF_INET, value.UTF8String, &addr) == 1;
}

+ (BOOL)isIPv6Address:(NSString *)value
{
    if (value.length == 0) {
        return NO;
    }
    struct in6_addr addr;
    return inet_pton(AF_INET6, value.UTF8String, &addr) == 1;
}

+ (NSString *)describeConnectionPath:(nw_connection_t)connection
{
    if (connection == NULL) {
        return @"path=(no connection)";
    }
    nw_path_t path = nw_connection_copy_current_path(connection);
    if (path == NULL) {
        return @"path=(none yet)";
    }

    const char *status = "?";
    switch (nw_path_get_status(path)) {
    case nw_path_status_invalid: status = "invalid"; break;
    case nw_path_status_satisfied: status = "satisfied"; break;
    case nw_path_status_unsatisfied: status = "unsatisfied"; break;
    case nw_path_status_satisfiable: status = "satisfiable"; break;
    }

    // The reason only carries meaning for an unsatisfied path; on a satisfied
    // one it still reads "not-available", which is pure noise in the log.
    const char *reason = "n/a";
    if (nw_path_get_status(path) == nw_path_status_unsatisfied) {
        if (@available(macOS 11.0, *)) {
            switch (nw_path_get_unsatisfied_reason(path)) {
            case nw_path_unsatisfied_reason_not_available: reason = "not-available"; break;
            case nw_path_unsatisfied_reason_cellular_denied: reason = "cellular-denied"; break;
            case nw_path_unsatisfied_reason_wifi_denied: reason = "wifi-denied"; break;
            case nw_path_unsatisfied_reason_local_network_denied: reason = "local-network-denied"; break;
            default: reason = "none"; break;
            }
        } else {
            reason = "?";
        }
    }

    NSMutableArray<NSString *> *interfaces = [NSMutableArray array];
    nw_path_enumerate_interfaces(path, ^bool(nw_interface_t interface) {
        [interfaces addObject:[NSString stringWithFormat:@"%s(type=%d)",
                                                         nw_interface_get_name(interface) ?: "?",
                                                         (int)nw_interface_get_type(interface)]];
        return true;
    });

    return [NSString stringWithFormat:@"path=%s reason=%s expensive=%d constrained=%d ifs=[%@]",
                                      status, reason,
                                      (int)nw_path_is_expensive(path),
                                      (int)nw_path_is_constrained(path),
                                      [interfaces componentsJoinedByString:@", "]];
}

+ (const char *)connectionStateName:(nw_connection_state_t)state
{
    switch (state) {
    case nw_connection_state_invalid: return "invalid";
    case nw_connection_state_waiting: return "waiting";
    case nw_connection_state_preparing: return "preparing";
    case nw_connection_state_ready: return "ready";
    case nw_connection_state_failed: return "failed";
    case nw_connection_state_cancelled: return "cancelled";
    default: return "unknown";
    }
}

+ (NSError *)errorFromNWError:(nw_error_t)error
{
    if (error == NULL) {
        return nil;
    }
    return CFBridgingRelease(nw_error_copy_cf_error(error));
}

@end
