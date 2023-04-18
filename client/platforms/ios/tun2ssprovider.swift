import Foundation
import Darwin

enum TunnelError: Error, Equatable {
    case noError
    case notFound(String)
    case undefinedError(String)
    case invalidServerCredentials(String)
    case serverUnreachable(String)
    case tunnelStartFailure(String)
    case illegalServerConfiguration(String)
    case systemMisconfigured(String)
    
    var desc: String {
        switch self {
        case .noError: return "OK. No errors."
        case .notFound(let msg): return msg
        case .undefinedError(let msg): return msg
        case .invalidServerCredentials(let msg): return msg
        case .serverUnreachable(let msg): return msg
        case .tunnelStartFailure(let msg): return msg
        case .illegalServerConfiguration(let msg): return msg
        case .systemMisconfigured(let msg): return msg
        }
    }
}

enum LeafProviderError: Int32 {
    case ok = 0
    case invalidConfigPath
    case invalidConfig
    case ioError
    case configFileWatcherError
    case asyncChannelSendError
    case asyncChannelRecvError
    case runtimeManagerError
    case noConfigFound
    
    static func toValue(from errorCode: Int32) -> LeafProviderError {
        switch errorCode {
        case ERR_OK: return ok
        case ERR_CONFIG_PATH: return invalidConfigPath
        case ERR_CONFIG: return invalidConfig
        case ERR_IO: return ioError
        case ERR_WATCHER: return configFileWatcherError
        case ERR_ASYNC_CHANNEL_SEND: return asyncChannelSendError
        case ERR_SYNC_CHANNEL_RECV: return asyncChannelRecvError
        case ERR_RUNTIME_MANAGER: return runtimeManagerError
        case ERR_NO_CONFIG_FILE: return noConfigFound
        default: return ok
        }
    }
    
    var desc: String {
        switch self {
        case .ok: return "Ok. No Errors."
        case .invalidConfigPath: return "Config file path is invalid."
        case .invalidConfig: return "Config parsing error."
        case .ioError: return "IO error."
        case .configFileWatcherError: return "Config file watcher error."
        case .asyncChannelSendError: return "Async channel send error."
        case .asyncChannelRecvError: return "Sync channel receive error."
        case .runtimeManagerError: return "Runtime manager error."
        case .noConfigFound: return "No associated config file."
        }
    }
}

class TunProvider: NSObject {
    private var configPath: UnsafePointer<CChar>
    private var tunId: UInt16
    private var tunThreadId: pthread_t? = nil
    private var dispatchQueue: DispatchQueue
    
    private var startCompletion: ((TunnelError?) -> Void)? = nil
    private var stopCompletion: ((TunnelError?) -> Void)? = nil
    
    init(withConfig configPath: UnsafePointer<CChar>) {
        self.configPath = configPath
        self.tunId = NSNumber(value: arc4random_uniform(UInt32.max)).uint16Value
        self.dispatchQueue = DispatchQueue(label: "org.amnezia.ss-tun-openvpn")
        super.init()
    }
    
    func startTunnel(completion: @escaping (TunnelError?) -> Void) {
        self.startCompletion = completion
        guard tunThreadId == nil else {
            let errMsg = "Leaf tunnel detached thread has already been started with id \(tunThreadId!)"
            wg_log(.error, message: errMsg)
            startCompletion?(.tunnelStartFailure(errMsg))
            return
        }
        wg_log(.info, message: "Starting tunnel thread...")
        dispatchQueue.async {
            self.startTunnelThread()
        }
        startCompletion?(.noError)
    }
    
    private func startLeafTunnel() {
        let err = leaf_run(tunId, configPath)
        if (err != ERR_OK) {
            let errMsg = LeafProviderError.toValue(from: err).desc
            startCompletion?(.tunnelStartFailure(errMsg))
        }
    }
    
    private func startTunnelThread() {
        var attr: pthread_attr_t = .init()
        var err = pthread_attr_init(&attr)
        wg_log(.debug, message: "pthread_attr_init returned \(err)")
        if (err != 0) {
            let errMsg = "pthread_attr_init failed with error \(err)"
            wg_log(.debug, message: errMsg)
            startCompletion?(.tunnelStartFailure(errMsg))
            return
        }
        err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)
        wg_log(.debug, message: "pthread_attr_setdetachstate returned \(err)")
        if (err != 0) {
            let errMsg = "pthread_attr_setdetachstate failed with error \(err)"
            wg_log(.debug, message: errMsg)
            startCompletion?(.tunnelStartFailure(errMsg))
            return
        }
        let observer = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())
        err = pthread_create(&tunThreadId, &attr, { obs in
            let provider = Unmanaged<TunProvider>.fromOpaque(obs).takeUnretainedValue()
            provider.startLeafTunnel()
            return nil
        }, observer)
        wg_log(.debug, message: "pthread_create returned \(err)")
        if (err != 0) {
            let errMsg = "pthread_create failed with error \(err)"
            wg_log(.debug, message: errMsg)
            startCompletion?(.tunnelStartFailure(errMsg))
            return
        }
        err = pthread_attr_destroy(&attr)
        wg_log(.debug, message: "pthread_attr_destroy returned \(err)")
        if (err != 0) {
            let errMsg = "pthread_create failed with error \(err)"
            wg_log(.debug, message: errMsg)
            startCompletion?(.tunnelStartFailure(errMsg))
            return
        }
    }
    
    func reloadTunnel(withId tunId: UInt16) {
        let err = leaf_reload(tunId)
        if (err != ERR_OK) {
            let errMsg = LeafProviderError.toValue(from: err).desc
            self.startCompletion?(.systemMisconfigured(errMsg))
        }
    }
    
    func stopTunnel(completion: @escaping (TunnelError?) -> Void) {
        self.stopCompletion = completion
        guard tunThreadId != nil else {
            let errMsg = "Leaf tunnel is not initialized properly or not started/existed, tunnelId is missed"
            wg_log(.error, message: errMsg)
            stopCompletion?(.notFound(errMsg))
            return
        }
        
        dispatchQueue.async {
            let success = leaf_shutdown(self.tunId)
            if !success {
                let errMsg = "Tunnel cannot be stopped for some odd reason."
                self.stopCompletion?(.undefinedError(errMsg))
            }
            pthread_kill(self.tunThreadId!, SIGUSR1)
            self.stopCompletion?(.noError)
            self.tunThreadId = nil
            self.stopCompletion = nil
        }
    }
    
    func testConfig(onPath path: UnsafePointer<CChar>, completion: @escaping (TunnelError?) -> Void) {
        self.startCompletion = completion
        let err = leaf_test_config(configPath)
        if (err != ERR_OK) {
            let errMsg = LeafProviderError.toValue(from: err).desc
            startCompletion?(.illegalServerConfiguration(errMsg))
            return
        }
        startCompletion?(.noError)
    }
    
    
    
}
