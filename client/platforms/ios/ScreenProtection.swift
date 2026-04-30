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
    guard let window = UIApplication.shared.keyWindows.first,
          let rootView = window.rootViewController?.view else {
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
      disable(for: rootView)
    } else {
      enable(for: rootView)
    }
  }

  public func enable(for view: UIView) {
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      view.subviews.forEach {
        self.pairs.append(ProtectionPair(from: $0))
      }
    }
  }

  public func disable(for view: UIView) {
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

  init(from view: UIView) {
    let secureTextField = UITextField()
    secureTextField.backgroundColor = .clear
    secureTextField.translatesAutoresizingMaskIntoConstraints = false
    secureTextField.isSecureTextEntry = true

    view.insertSubview(secureTextField, at: 0)
    secureTextField.isUserInteractionEnabled = false

    view.layer.superlayer?.addSublayer(secureTextField.layer)
    secureTextField.layer.sublayers?.last?.addSublayer(view.layer)

    secureTextField.topAnchor.constraint(equalTo: view.topAnchor, constant: 0).isActive = true
    secureTextField.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: 0).isActive = true
    secureTextField.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 0).isActive = true
    secureTextField.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: 0).isActive = true

    self.init(textField: secureTextField, layer: view.layer)
  }

  init(textField: UITextField, layer: CALayer) {
    self.textField = textField
    self.layer = layer
  }

  func removeProtection() {
    textField.superview?.superview?.layer.addSublayer(layer)
    textField.layer.removeFromSuperlayer()
    textField.removeFromSuperview()
  }
}
#endif
