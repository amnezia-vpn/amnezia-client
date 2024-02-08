/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import os.log
import OSLog

public class IOSLogger : NSObject {
    struct Constants {
    #if os(iOS)
        static let appGroupIdentifier = "com.wireguard.ios.app_group_id"
    #elseif os(macOS)
        static let appGroupIdentifier = "com.wireguard.macos.app_group_id"
    #endif
        static let networkExtensionLogFileName = "networkextension.log"
    }
    
    private let log: OSLog
    
    private lazy var dateFormatter: DateFormatter = {
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "dd.MM.yyyy HH:mm:ss"
        return dateFormatter
    }()
    
    private static let logger = IOSLogger(tag: "IOSLogger")
    private static var appexLogFileURL: URL? {
        get {
            guard let containerURL = FileManager.default.containerURL(
                forSecurityApplicationGroupIdentifier: Constants.appGroupIdentifier
            ) else {
                return nil
            }
            
            return containerURL.appendingPathComponent(Constants.networkExtensionLogFileName, isDirectory: false)
        }
    }

    @objc init(tag: String) {
        self.log = OSLog(
            subsystem: Bundle.main.bundleIdentifier!,
            category: tag
        )
    }

    @objc func debug(message: String) {
        log(message, type: .debug)
    }

    @objc func info(message: String) {
        log(message, type: .info)
    }

    @objc func error(message: String) {
        log(message, type: .error)
    }

    func log(_ message: String, type: OSLogType) {
        os_log("%{public}@", log: self.log, type: type, message)
                
//        if (Bundle.main.bundlePath.hasSuffix(".appex")) {
//            let currentDate = Date()
//            let formattedDateString = dateFormatter.string(from: currentDate)
//
//            if let data = "[\(formattedDateString)] \(message)\n".data(using: .utf8) {
//                let _ = IOSLogger.withAppexLogFile { logFileHandle in
//                    logFileHandle.seekToEndOfFile()
//                    logFileHandle.write(data)
//                }
//            }
//        }
    }
    
    @objc static func getAppexLogs(callback: @escaping (String) -> Void) {
        withAppexLogFile { logFileHandle in
            if let contents = String(data: logFileHandle.readDataToEndOfFile(), encoding: .utf8) {
                callback(contents);
            }
        }
    }
    
    @objc static func clearAppexLogs() {
        withAppexLogFile { logFileHandle in
            logFileHandle.truncateFile(atOffset: 0)
        }
    }
    
    private static func withAppexLogFile(_ f: (_ handle: FileHandle) throws -> Void) {
        guard let appexLogFileURL = IOSLogger.appexLogFileURL else {
            logger.error(message: "IMPOSSIBLE: No known app extension log file.")
            return
        }

        
        do {
            if !FileManager.default.fileExists(atPath: appexLogFileURL.path) {
                // Create an empty file
                if let data = "".data(using: .utf8) {
                    try data.write(to: appexLogFileURL)
                } else {
                    logger.error(message: "Unable to create log file at \(appexLogFileURL)")
                    return
                }
            }
            
            let fileHandle = try FileHandle(forUpdating: appexLogFileURL)
            try f(fileHandle)
            fileHandle.closeFile()
        } catch {
            logger.error(message: "Unable to access log file at \(appexLogFileURL): \(error)")
        }
    }
}

// The following functions are used by Wireguard internally for logging.

let wireguardLogger = IOSLogger(tag: "Wireguard")

func wg_log(_ type: OSLogType, staticMessage: StaticString) {
    wireguardLogger.log("\(staticMessage)", type: type)
}

func wg_log(_ type: OSLogType, message: String) {
    wireguardLogger.log(message, type: type)
}
