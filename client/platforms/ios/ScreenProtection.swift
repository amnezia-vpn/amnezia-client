#if MACOS_NE
public func toggleScreenshots(_ isEnabled: Bool) {
  
}

class ScreenProtection {


}
#else
import UIKit

public func toggleScreenshots(_ isEnabled: Bool) {
  ScreenProtection.shared.setScreenshotsEnabled(isEnabled)
}

extension UIApplication {
  var keyWindows: [UIWindow] {
    connectedScenes
      .compactMap {
        guard let windowScene = $0 as? UIWindowScene else { return nil }
        if #available(iOS 15.0, *) {
          guard let keywindow = windowScene.keyWindow else {
            windowScene.windows.first?.makeKey()
            return windowScene.windows.first
          }
          return keywindow
        } else {
          return windowScene.windows.first { $0.isKeyWindow }
        }
      }
  }
}

class ScreenProtection {
  public static let shared = ScreenProtection()

  var pairs = [ProtectionPair]()

  private var blurView: UIVisualEffectView?
  private var recordingObservation: NSKeyValueObservation?
  private var desiredScreenshotsEnabled: Bool?
  private var retryCount = 0
  private var retryWorkItem: DispatchWorkItem?

  public func setScreenshotsEnabled(_ isEnabled: Bool) {
    DispatchQueue.main.async {
      self.desiredScreenshotsEnabled = isEnabled
      self.applyScreenshotsSettingOrRetry()
    }
  }

  private func applyScreenshotsSettingOrRetry() {
    assert(Thread.isMainThread)

    guard let desiredScreenshotsEnabled else { return }
    guard let window = UIApplication.shared.keyWindows.first else {
      retryCount += 1
      guard retryCount <= 50 else { return } // ~5s total

      retryWorkItem?.cancel()
      let item = DispatchWorkItem { [weak self] in
        self?.applyScreenshotsSettingOrRetry()
      }
      retryWorkItem = item
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1, execute: item)
      return
    }

    retryWorkItem?.cancel()
    retryWorkItem = nil
    retryCount = 0

    if desiredScreenshotsEnabled {
      disable()
    } else {
      enable(for: window)
    }
  }

  public func enable(for window: UIWindow) {
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      self.pairs.append(ProtectionPair(from: window))
    }
  }

  public func disable() {
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      self.pairs.forEach {
        $0.removeProtection()
      }

      self.pairs.removeAll()
    }
  }
}

struct ProtectionPair {
  let textField: UITextField
  let layer: CALayer
  let originalSuperlayer: CALayer?

  init(from view: UIView) {
    let secureTextField = UITextField()
    secureTextField.backgroundColor = .clear
    secureTextField.isSecureTextEntry = true

    view.insertSubview(secureTextField, at: 0)
    secureTextField.isUserInteractionEnabled = false

    let originalSuperlayer = view.layer.superlayer
    originalSuperlayer?.addSublayer(secureTextField.layer)
    secureTextField.layer.sublayers?.last?.addSublayer(view.layer)

    self.init(textField: secureTextField, layer: view.layer, originalSuperlayer: originalSuperlayer)
  }

  init(textField: UITextField, layer: CALayer, originalSuperlayer: CALayer?) {
    self.textField = textField
    self.layer = layer
    self.originalSuperlayer = originalSuperlayer
  }

  func removeProtection() {
    originalSuperlayer?.addSublayer(layer)
    textField.layer.removeFromSuperlayer()
    textField.removeFromSuperview()
  }
}
#endif
