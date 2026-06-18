import Foundation
import AppIntents

@available(iOS 16.0, *)
public struct IntentCallbacks {
    public static var reload: (@convention(c) () -> Void)? = nil
    public static var connect: (@convention(c) (UnsafePointer<CChar>) -> Void)? = nil
    public static var getCountries: (@convention(c) () -> UnsafePointer<CChar>)? = nil
}

@_cdecl("set_intent_callbacks")
public func setIntentCallbacks(
    reloadCallback: @escaping @convention(c) () -> Void,
    connectCallback: @escaping @convention(c) (UnsafePointer<CChar>) -> Void,
    getCountriesCallback: @escaping @convention(c) () -> UnsafePointer<CChar>
) {
    if #available(iOS 16.0, *) {
        IntentCallbacks.reload = reloadCallback
        IntentCallbacks.connect = connectCallback
        IntentCallbacks.getCountries = getCountriesCallback
    }
}

@available(iOS 16.0, *)
func waitForCallbacks() async {
    // Wait up to 10 seconds for Qt app to initialize
    for _ in 0..<100 {
        if IntentCallbacks.reload != nil { return }
        try? await Task.sleep(nanoseconds: 100_000_000)
    }
}

@available(iOS 16.0, *)
struct ReloadApiConfigIntent: AppIntent {
    static var title: LocalizedStringResource = "Reload API Config"
    static var description = IntentDescription("Reloads Amnezia VPN API Configuration.")
    
    func perform() async throws -> some IntentResult {
        await waitForCallbacks()
        if let reload = IntentCallbacks.reload {
            reload()
        }
        return .result()
    }
}

@available(iOS 16.0, *)
struct CountryOption: AppEntity {
    var id: String
    var name: String
    
    static var typeDisplayRepresentation: TypeDisplayRepresentation = "Country"
    
    var displayRepresentation: DisplayRepresentation {
        DisplayRepresentation(title: "\(name)")
    }
    
    static var defaultQuery = CountryOptionQuery()
}

@available(iOS 16.0, *)
struct CountryOptionQuery: EntityQuery {
    func entities(for identifiers: [String]) async throws -> [CountryOption] {
        let all = try await suggestedEntities()
        return all.filter { identifiers.contains($0.id) }
    }
    
    func suggestedEntities() async throws -> [CountryOption] {
        await waitForCallbacks()
        var options: [CountryOption] = []
        if let getCountries = IntentCallbacks.getCountries {
            let jsonStringPtr = getCountries()
            let jsonString = String(cString: jsonStringPtr)
            if let data = jsonString.data(using: .utf8),
               let array = try? JSONSerialization.jsonObject(with: data, options: []) as? [[String: String]] {
                for item in array {
                    if let code = item["code"], let name = item["name"] {
                        options.append(CountryOption(id: code, name: name))
                    }
                }
            }
        }
        return options
    }
}

@available(iOS 16.0, *)
struct ConnectToCountryIntent: AppIntent {
    static var title: LocalizedStringResource = "Connect to Country"
    static var description = IntentDescription("Connects Amnezia VPN to a specific country.")
    
    @Parameter(title: "Country")
    var country: CountryOption
    
    func perform() async throws -> some IntentResult {
        await waitForCallbacks()
        if let connect = IntentCallbacks.connect {
            let code = country.id
            code.withCString { cStr in
                connect(cStr)
            }
        }
        return .result()
    }
}

@available(iOS 16.0, *)
struct AmneziaShortcutsProvider: AppShortcutsProvider {
    static var appShortcuts: [AppShortcut] {
        AppShortcut(
            intent: ReloadApiConfigIntent(),
            phrases: [
                "Reload \(.applicationName) API Config",
                "Refresh \(.applicationName) Configuration"
            ],
            shortTitle: "Reload API Config",
            systemImageName: "arrow.clockwise"
        )
        
        AppShortcut(
            intent: ConnectToCountryIntent(),
            phrases: [
                "Connect to Country in \(.applicationName)",
                "Connect \(.applicationName) to Country"
            ],
            shortTitle: "Connect to Country",
            systemImageName: "network"
        )
    }
}
