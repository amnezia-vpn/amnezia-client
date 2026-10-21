import QtQuick

import "../Controls2"

CaptchaDialogType {
    id: root

    property bool active: true

    Connections {
        target: SubscriptionUiController
        enabled: root.active

        function onFreeAccountCaptchaRequired(captchaId, captchaImageBase64, hint) {
            root.captchaId = captchaId
            root.captchaImageBase64 = captchaImageBase64
            if (hint !== "") {
                root.hint = hint
            }
            if (!root.opened) {
                root.open()
            }
        }

        function onFreeAccountCaptchaPassed() {
            if (root.opened) {
                root.close()
            }
        }

        function onFreeAccountLoginFinished() {
            if (root.opened) {
                root.close()
            }
        }

        function onFreeAccountLoginFailed(kind, retryAfterSec) {
            if (root.opened && kind !== "captchaInvalid" && kind !== "captchaRetry") {
                root.close()
            }
        }
    }

    onCaptchaSolved: function(captchaId, solution) {
        PageController.showBusyIndicator(true)
        Qt.callLater(function() {
            SubscriptionUiController.solveFreeAccountCaptcha(solution)
            PageController.showBusyIndicator(false)
        })
    }

    onRefreshCaptchaRequested: function() {
        PageController.showBusyIndicator(true)
        Qt.callLater(function() {
            SubscriptionUiController.refreshFreeAccountCaptcha()
            PageController.showBusyIndicator(false)
        })
    }
}
