import Foundation
import ShadowSocks
import NetworkExtension
import Darwin

enum ErrorCode: Int {
    case noError = 0
    case undefinedError
    case vpnPermissionNotGranted
    case invalidServerCredentials
    case udpRelayNotEnabled
    case serverUnreachable
    case vpnStartFailure
    case illegalServerConfiguration
    case shadowsocksStartFailure
    case configureSystemProxyFailure
    case noAdminPermissions
    case unsupportedRoutingTable
    case systemMisconfigured
}

protocol ShadowSocksTunnel {
    var ssLocalThreadId: pthread_t? { get }
    func start(usingPacketFlow packetFlow: ShadowSocksAdapterPacketFlow,
               withConnectivityCheck connectivityCheck: Bool,
               completion: @escaping (ErrorCode?) -> Void)
    func stop(completion: @escaping (ErrorCode?) -> Void)
    func isUp() -> Bool
}

final class SSProvider: NSObject, ShadowSocksTunnel {
    private var startCompletion: ((ErrorCode?) -> Void)? = nil
    private var stopCompletion: ((ErrorCode?) -> Void)? = nil
    
    private var dispatchQueue: DispatchQueue
    private var processQueue: DispatchQueue
    private var dispatchGroup: DispatchGroup
    
    private var connectivityCheck: Bool = false
    private var ssConnectivity: ShadowsocksConnectivity? = nil
    
    private var config: Data
    
    private var ssPacketFlowBridge: ShadowSocksAdapterFlowBridge? = nil
    
    var ssLocalThreadId: pthread_t? = nil
    
    init(config: Data, localPort: Int = 8585) {
        self.config = config
        self.dispatchQueue = DispatchQueue(label: "org.amnezia.shadowsocks")
        self.processQueue = DispatchQueue(label: "org.amnezia.packet-processor")
        self.dispatchGroup = DispatchGroup()
        self.ssPacketFlowBridge = .init()
        self.ssConnectivity = ShadowsocksConnectivity.init(port: NSNumber(value: localPort).uint16Value)
        super.init()
    }
    
    func isUp() -> Bool {
        return ssLocalThreadId != nil
    }
    
    func start(usingPacketFlow packetFlow: ShadowSocksAdapterPacketFlow, withConnectivityCheck connectivityCheck: Bool = false, completion: @escaping (ErrorCode?) -> Void) {
        guard ssLocalThreadId == nil else {
            wg_log(.error, message: "SS detached thread has already been started with id \(ssLocalThreadId!)")
            completion(.shadowsocksStartFailure)
            return
        }
        
        wg_log(.info, message: "Starting SSProvider...")
        self.connectivityCheck = connectivityCheck
        wg_log(.info, message: "ssPacketFlowBridge is \(ssPacketFlowBridge != nil ? "not null" : "null")")
        self.ssPacketFlowBridge?.ssPacketFlow = packetFlow
        wg_log(.info, message: "ssPacketFlow is \(ssPacketFlowBridge?.ssPacketFlow != nil ? "not null" : "null")")
        self.startCompletion = completion
        dispatchQueue.async {
            wg_log(.info, message: "Starting ss thread...")
            self.startShadowSocksThread()
        }
    }
    
    func stop(completion: @escaping (ErrorCode?) -> Void) {
        guard ssLocalThreadId != nil else { return }
        self.stopCompletion = completion
        dispatchQueue.async {
            pthread_kill(self.ssLocalThreadId!, SIGUSR1)
            self.ssPacketFlowBridge?.invalidateSocketsIfNeeded()
            self.ssPacketFlowBridge = nil
            self.ssLocalThreadId = nil
        }
    }
    
    private func onShadowsocksCallback(socks_fd: Int32, udp_fd: Int32, obs: UnsafeMutableRawPointer) {
        NSLog("Inside onShadowsocksCallback() with port \(socks_fd)")
        wg_log(.debug, message: "Inside onShadowsocksCallback() with port \(socks_fd)")
        var error: NSError? = nil
        if (socks_fd <= 0 && udp_fd <= 0) {
            error = NSError(domain: Bundle.main.bundleIdentifier ?? "unknown", code: 100, userInfo: [NSLocalizedDescriptionKey : "Failed to start shadowsocks proxy"])
            NSLog("onShadowsocksCallback with error \(error!.localizedDescription)")
            wg_log(.debug, message: "onShadowsocksCallback failed with error \(error!.localizedDescription)")
//            return
        }
        let ss = Unmanaged<SSProvider>.fromOpaque(obs).takeUnretainedValue()
        ss.establishTunnel()
        ss.checkServerConnectivity()
    }
    
    private func startShadowSocksThread() {
        var attr: pthread_attr_t = .init()
        var err = pthread_attr_init(&attr)
        wg_log(.debug, message: "pthread_attr_init returned \(err)")
        if (err != 0) {
            NSLog("pthread_attr_init failed with error \(err)")
            wg_log(.debug, message: "pthread_attr_init failed with error \(err)")
            startCompletion?(.shadowsocksStartFailure)
            return
        }
        err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)
        wg_log(.debug, message: "pthread_attr_setdetachstate returned \(err)")
        if (err != 0) {
            wg_log(.debug, message: "pthread_attr_setdetachstate failed with error \(err)")
            startCompletion?(.shadowsocksStartFailure)
            return
        }
        let observer = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())
        err = pthread_create(&ssLocalThreadId, &attr, { obs in
            let ss = Unmanaged<SSProvider>.fromOpaque(obs).takeUnretainedValue()
            ss.startShadowSocks()
            return nil
        }, observer)
        wg_log(.debug, message: "pthread_create returned \(err)")
        if (err != 0) {
            NSLog("pthread_create failed with error \(err)")
            wg_log(.debug, message: "pthread_create failed with error \(err)")
            startCompletion?(.shadowsocksStartFailure)
            return
        }
        err = pthread_attr_destroy(&attr)
        wg_log(.debug, message: "pthread_attr_destroy returned \(err)")
        if (err != 0) {
            NSLog("pthread_create failed with error \(err)")
            wg_log(.debug, message: "pthread_attr_destroy failed with error \(err)")
            startCompletion?(.shadowsocksStartFailure)
            return
        }
    }
    
    private func startShadowSocks() {
        wg_log(.debug, message: "startShadowSocks with config \(config)")
        let str = String(decoding: config, as: UTF8.self)
        wg_log(.info, message: "startShadowSocks -> config: \(str)")
        guard let ssConfig = try? JSONSerialization.jsonObject(with: config, options: []) as? [String: Any] else {
            startCompletion?(.configureSystemProxyFailure)
            return
        }

        wg_log(.info, message: "SS Config: \(ssConfig)")

        guard let remoteHost = ssConfig["server"] as? String, // UnsafeMutablePointer<CChar>,
              let remotePort = ssConfig["server_port"] as? Int32,
              let localAddress = ssConfig["local_addr"] as? String, //UnsafeMutablePointer<CChar>,
              let localPort = ssConfig["local_port"] as? Int32,
              let method = ssConfig["method"] as? String, //UnsafeMutablePointer<CChar>,
              let password = ssConfig["password"] as? String,//UnsafeMutablePointer<CChar>,
              let timeout = ssConfig["timeout"] as? Int32
        else {
            startCompletion?(.configureSystemProxyFailure)
            return
        }

        /* An example profile
         *
         * const profile_t EXAMPLE_PROFILE = {
         *  .remote_host = "example.com",
         *  .local_addr = "127.0.0.1",
         *  .method = "bf-cfb",
         *  .password = "barfoo!",
         *  .remote_port = 8338,
         *  .local_port = 1080,
         *  .timeout = 600;
         *  .acl = NULL,
         *  .log = NULL,
         *  .fast_open = 0,
         *  .mode = 0,
         *  .verbose = 0
         * };
         */

        var profile: profile_t = .init()
        memset(&profile, 0, MemoryLayout<profile_t>.size)
        profile.remote_host = strdup(remoteHost)
        profile.remote_port = remotePort
        profile.local_addr = strdup(localAddress)
        profile.local_port = localPort
        profile.method = strdup(method)
        profile.password = strdup(password)
        profile.timeout = timeout
        profile.acl = nil
        profile.log = nil
//        profile.mtu = 1600
        profile.fast_open = 1
        profile.mode = 0
        profile.verbose = 1
        
        NSLog("Prepare to start shadowsocks proxy server...")
        wg_log(.debug, message: "Prepare to start shadowsocks proxy server...")
        let observer = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())
        let success = start_ss_local_server_with_callback(profile, { socks_fd,  udp_fd, data in
            wg_log(.debug, message: "Inside cb callback")
            wg_log(.debug, message: "Params: socks_fd -> \(socks_fd), udp_fd -> \(udp_fd)")
            NSLog("Inside cb callback: Params: socks_fd -> \(socks_fd), udp_fd -> \(udp_fd)")
            if let obs = data {
                NSLog("Prepare to call onShadowsocksCallback() with socks port \(socks_fd) and udp port \(udp_fd)")
                wg_log(.debug, message: "Prepare to call onShadowsocksCallback() with socks port \(socks_fd) and udp port \(udp_fd)")
                let mySelf = Unmanaged<SSProvider>.fromOpaque(obs).takeUnretainedValue()
                mySelf.onShadowsocksCallback(socks_fd: socks_fd, udp_fd: udp_fd, obs: obs)
            }
        }, observer)
        if success < 0 {
            NSLog("Failed to start ss proxy")
            wg_log(.error, message: "Failed to start ss proxy")
            startCompletion?(.shadowsocksStartFailure)
            return
        } else {
            NSLog("ss proxy started on port \(localPort)")
            wg_log(.error, message: "ss proxy started on port \(localPort)")
            stopCompletion?(.noError)
            stopCompletion = nil
        }
    }
    
    private func establishTunnel() {
        wg_log(.error, message: "Establishing tunnel")
        do {
            try ssPacketFlowBridge?.configureSocket()
            processQueue.async {
                wg_log(.error, message: "Start processing packets")
                self.ssPacketFlowBridge?.processPackets()
            }
        } catch (let err) {
            wg_log(.error, message: "ss failed creating sockets \(err.localizedDescription)")
            ssPacketFlowBridge?.invalidateSocketsIfNeeded()
        }
    }
    
    private func checkServerConnectivity() {
        guard connectivityCheck else {
            startCompletion?(.noError)
            return
        }
        
        let str = String(decoding: config, as: UTF8.self)
        wg_log(.info, message: "checkServerConnectivity -> config: \(str)")
        guard let ssConfig = try? JSONSerialization.jsonObject(with: config, options: []) as? [String: Any],
              let remoteHost = ssConfig["server"] as? String,
              let remotePort = ssConfig["server_port"] as? Int32 else {
            startCompletion?(.configureSystemProxyFailure)
            return
        }
        
        var isRemoteUdpForwardingEnabled = false
        var serverCredentialsAreValid = false
        var isServerReachable = false
        
        dispatchGroup.enter()
        ssConnectivity?.isUdpForwardingEnabled { status in
            isRemoteUdpForwardingEnabled = status
            self.dispatchGroup.leave()
        }
        dispatchGroup.enter()
        ssConnectivity?.isReachable(remoteHost, port: NSNumber(value: remotePort).uint16Value) { status in
            isServerReachable = status
            self.dispatchGroup.leave()
        }
        dispatchGroup.enter()
        ssConnectivity?.checkServerCredentials{ status in
            serverCredentialsAreValid = status
            self.dispatchGroup.leave()
        }
        
        dispatchGroup.notify(queue: dispatchQueue) {
            if isRemoteUdpForwardingEnabled {
                self.startCompletion?(.noError)
            } else if serverCredentialsAreValid {
                self.startCompletion?(.udpRelayNotEnabled)
            } else if isServerReachable {
                self.startCompletion?(.invalidServerCredentials)
            } else {
                self.startCompletion?(.serverUnreachable)
            }
        }
    }
}
