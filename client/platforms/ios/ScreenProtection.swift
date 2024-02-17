import UIKit

public func toggleScreenshots(_ isEnabled: Bool) {
  let window = UIApplication.shared.keyWindows.first!

  if isEnabled {
    ScreenProtection.shared.disable(for: window.rootViewController!.view)
  } else {
    ScreenProtection.shared.enable(for: window.rootViewController!.view)
  }
}

extension UIApplication {
  var keyWindows: [UIWindow] {
    connectedScenes
      .compactMap {
        if #available(iOS 15.0, *) {
          ($0 as? UIWindowScene)?.keyWindow
        } else {
          ($0 as? UIWindowScene)?.windows.first { $0.isKeyWindow }
        }
      }
  }
}

class ScreenProtection {
  public static let shared = ScreenProtection()

  var pairs = [ProtectionPair]()

  private var blurView: UIVisualEffectView?
  private var recordingObservation: NSKeyValueObservation?

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

//#if os(iOS)
    view.layer.superlayer?.addSublayer(secureTextField.layer)
    secureTextField.layer.sublayers?.last?.addSublayer(view.layer)

    secureTextField.topAnchor.constraint(equalTo: view.topAnchor, constant: 0).isActive = true
    secureTextField.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: 0).isActive = true
    secureTextField.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 0).isActive = true
    secureTextField.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: 0).isActive = true
//#else
//    secureTextField.frame = bounds
//    secureTextField.wantsLayer = true
//    secureTextField.layer?.addSublayer(layer!)
//    addSubview(secureTextField)
//#endif

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
