import Foundation
import NetworkExtension
import Tun2socks

class AmneziaTun2SocksWriter: NSObject, Tun2socksTunWriterProtocol {
    var tunnelFlow: NEPacketTunnelFlow

    init( withPacketFlow nepflow: NEPacketTunnelFlow) {
        self.tunnelFlow = nepflow
        super.init()
    }

    func write(_ p0: Data?, n: UnsafeMutablePointer<Int>?) throws {
        if let packets = p0 {
            tunnelFlow.writePackets([packets], withProtocols: [NSNumber(value: PF_INET)])
        }
    }

    func close() throws {}
}

