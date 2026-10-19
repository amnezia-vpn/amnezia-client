import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

Popup {
    id: root

    // captcha | email | code
    property string step: "captcha"
    property string email: ""
    property bool busy: false
    property int resendCooldownSec: 0
    property int codeTtlSec: 0
    property string captchaId: ""
    property string captchaImageBase64: ""
    property string captchaHint: ""

    readonly property int defaultResendCooldownSec: 60
    readonly property int defaultCodeTtlSec: 300
    readonly property bool captchaStep: root.step === "captcha"
    readonly property bool emailStep: root.step === "email"
    readonly property bool codeStep: root.step === "code"
    readonly property bool codeExpired: root.codeStep && root.codeTtlSec <= 0

    readonly property int keyboardHeight: {
        if (PageController.imeHeight > 0) {
            return PageController.imeHeight
        }
        if (Qt.platform.os === "ios" && Qt.inputMethod.visible) {
            const r = Qt.inputMethod.keyboardRectangle
            if (r.height <= 0) {
                return 0
            }
            return Math.max(0, Math.round(parent.height - r.y))
        }
        return 0
    }

    function start() {
        root.step = "captcha"
        root.email = ""
        root.resendCooldownSec = 0
        root.codeTtlSec = 0
        root.captchaId = ""
        root.captchaImageBase64 = ""
        root.captchaHint = ""
        captchaField.errorText = ""
        captchaField.textField.text = ""
        emailField.errorText = ""
        codeField.errorText = ""
        codeField.textField.text = ""
        root.open()
        root.runBusy(function() {
            SubscriptionUiController.startFreeAccountLogin()
        })
    }

    function isValidEmail(value) {
        return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value)
    }

    function runBusy(action) {
        root.busy = true
        Qt.callLater(function() {
            action()
            root.busy = false
        })
    }

    function solveCaptcha() {
        const value = captchaField.textField.text.trim()
        if (value === "") {
            captchaField.errorText = qsTr("Enter the digits from the image")
            return
        }
        root.runBusy(function() {
            SubscriptionUiController.solveFreeAccountCaptcha(value)
        })
    }

    function refreshCaptcha() {
        captchaField.errorText = ""
        captchaField.textField.text = ""
        root.runBusy(function() {
            SubscriptionUiController.refreshFreeAccountCaptcha()
        })
    }

    function sendCode() {
        const value = emailField.textField.text.trim()
        if (!root.isValidEmail(value)) {
            emailField.errorText = qsTr("Enter a valid email address")
            return
        }
        root.email = value
        root.runBusy(function() {
            SubscriptionUiController.requestFreeEmailCode(value)
        })
    }

    function resendCode() {
        if (root.busy || root.resendCooldownSec > 0) {
            return
        }
        codeField.errorText = ""
        root.runBusy(function() {
            SubscriptionUiController.resendFreeEmailCode()
        })
    }

    function confirmCode() {
        const value = codeField.textField.text.trim()
        if (value === "") {
            codeField.errorText = qsTr("Enter the code from the email")
            return
        }
        root.runBusy(function() {
            SubscriptionUiController.confirmFreeEmailCode(value)
        })
    }

    function submit() {
        if (root.captchaStep) {
            root.solveCaptcha()
        } else if (root.emailStep) {
            root.sendCode()
        } else {
            root.confirmCode()
        }
    }

    function focusCurrentField() {
        if (root.captchaStep) {
            captchaField.textField.forceActiveFocus()
        } else if (root.emailStep) {
            emailField.textField.forceActiveFocus()
        } else {
            codeField.textField.forceActiveFocus()
        }
    }

    function startResendCooldown(seconds) {
        root.resendCooldownSec = seconds > 0 ? seconds : root.defaultResendCooldownSec
    }

    function formattedTime(totalSeconds) {
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function errorMessage(kind, retryAfterSec) {
        switch (kind) {
        case "captchaInvalid": return qsTr("CAPTCHA was incorrect. Please try again")
        case "captchaRetry": return qsTr("Please solve the new CAPTCHA to continue")
        case "noNetwork": return qsTr("No connection. Check your internet and try again")
        case "rateLimited": return retryAfterSec > 0 ? qsTr("Too many requests. Try again in %1 s").arg(retryAfterSec)
                                                     : qsTr("Too many requests. Try again later")
        case "invalidCode": return qsTr("The code is invalid or has expired. Request a new one")
        case "retry": return qsTr("Something went wrong. Please try again")
        case "config": return qsTr("Couldn't create the configuration. Please try again")
        default: return qsTr("Something went wrong. Try again later")
        }
    }

    parent: Overlay.overlay

    width: parent ? parent.width : 0
    height: Math.min(contentLayout.implicitHeight + 21 + 16 + 8 + PageController.safeAreaBottomMargin, parent ? parent.height - 20 : 0)

    x: 0
    y: parent ? parent.height - height - root.keyboardHeight : 0

    padding: 0

    Behavior on y {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    modal: true
    closePolicy: root.busy ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.bgOverlay
    }

    focus: true

    onOpened: root.focusCurrentField()
    onStepChanged: root.focusCurrentField()

    onClosed: {
        SubscriptionUiController.cancelFreeAccountLogin()
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
    }

    Timer {
        id: ticker

        interval: 1000
        repeat: true
        running: root.opened && (root.resendCooldownSec > 0 || root.codeTtlSec > 0)

        onTriggered: {
            if (root.resendCooldownSec > 0) {
                root.resendCooldownSec -= 1
            }
            if (root.codeTtlSec > 0) {
                root.codeTtlSec -= 1
            }
        }
    }

    Connections {
        target: SubscriptionUiController

        function onFreeAccountCaptchaRequired(captchaId, captchaImageBase64, hint) {
            if (!root.opened) {
                return
            }
            root.captchaId = captchaId
            root.captchaImageBase64 = captchaImageBase64
            root.captchaHint = hint
            captchaField.textField.text = ""
            captchaField.errorText = ""
            root.step = "captcha"
            root.focusCurrentField()
        }

        function onFreeAccountCaptchaPassed() {
            if (!root.opened) {
                return
            }
            root.step = "email"
        }

        function onFreeEmailCodeSent(retryAfterSec) {
            if (!root.opened) {
                return
            }
            codeField.textField.text = ""
            codeField.errorText = ""
            root.step = "code"
            root.codeTtlSec = root.defaultCodeTtlSec
            root.startResendCooldown(retryAfterSec)
        }

        function onFreeAccountLoginFailed(kind, retryAfterSec) {
            if (!root.opened) {
                return
            }
            if (kind === "") {
                root.close()
                return
            }
            const message = root.errorMessage(kind, retryAfterSec)
            if (root.captchaStep) {
                captchaField.errorText = message
            } else if (root.emailStep) {
                emailField.errorText = message
            } else {
                codeField.errorText = message
            }
            if (kind === "rateLimited") {
                root.startResendCooldown(retryAfterSec)
            }
        }

        function onFreeAccountLoginFinished() {
            if (root.opened) {
                root.close()
            }
        }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -border.width

            topLeftRadius: 16
            topRightRadius: 16
            color: AmneziaStyle.color.bgSurfaceAlt
            border.color: AmneziaStyle.color.borderFaint
            border.width: 1
        }

        SheetGrabberType {
            anchors.top: parent.top
            anchors.topMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    contentItem: Item {
        ColumnLayout {
            id: contentLayout

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 21 + 16
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 0

            H2TextType {
                Layout.fillWidth: true

                text: {
                    if (root.captchaStep) {
                        return root.captchaHint !== "" ? root.captchaHint : qsTr("Enter the digits from the image to continue")
                    }
                    return root.emailStep ? qsTr("Sign in with email") : qsTr("Enter the code")
                }
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8

                visible: !root.captchaStep

                text: root.emailStep
                      ? qsTr("We'll send a one-time sign-in code to this address and create an account for your free subscription")
                      : qsTr("We've sent a sign-in code to %1").arg(root.email)
            }

            Rectangle {
                id: imagePanel

                Layout.fillWidth: true
                Layout.preferredHeight: 128
                Layout.topMargin: 24

                visible: root.captchaStep

                color: AmneziaStyle.color.bgSurfaceInverse
                radius: 16
                clip: true

                Image {
                    id: captchaImage

                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    cache: false

                    source: root.captchaImageBase64 !== "" ? "data:image/png;base64," + root.captchaImageBase64 : ""
                }

                SpinnerType {
                    anchors.centerIn: parent
                    color: AmneziaStyle.color.textInverted
                    visible: captchaImage.status === Image.Loading || (root.busy && root.captchaImageBase64 === "")
                }

                IconButtonType {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 16
                    anchors.bottomMargin: 12

                    enabled: !root.busy
                    filled: true
                    icon.source: "qrc:/images/controls/refresh-cw.svg"

                    onClicked: root.refreshCaptcha()
                }
            }

            TextFieldType {
                id: captchaField

                Layout.fillWidth: true
                Layout.topMargin: 24

                visible: root.captchaStep

                headerText: qsTr("Digits from the image")
                textField.placeholderText: qsTr("_ _ _ _ _ _")
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                textField.maximumLength: 6
                textField.validator: RegularExpressionValidator { regularExpression: /^[0-9]{0,6}$/ }
                textField.font.letterSpacing: 2

                textField.onAccepted: root.solveCaptcha()
            }

            TextFieldType {
                id: emailField

                Layout.fillWidth: true
                Layout.topMargin: 24

                visible: root.emailStep

                headerText: qsTr("Email")
                textField.placeholderText: qsTr("Email")
                textField.inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

                textField.onAccepted: root.sendCode()
            }

            TextFieldType {
                id: codeField

                Layout.fillWidth: true
                Layout.topMargin: 24

                visible: root.codeStep

                headerText: qsTr("Code from the email")
                textField.placeholderText: qsTr("Code")
                textField.inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
                textField.font.letterSpacing: 2

                textField.onAccepted: root.confirmCode()
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 4

                visible: root.codeStep

                color: root.codeExpired ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textTertiary
                text: root.codeExpired ? qsTr("The code has expired. Request a new one")
                                       : qsTr("Time left %1").arg(root.formattedTime(root.codeTtlSec))
            }

            ButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 12

                enabled: {
                    if (root.busy) {
                        return false
                    }
                    if (root.captchaStep) {
                        return root.captchaImageBase64 !== ""
                    }
                    return root.emailStep ? root.resendCooldownSec <= 0 : !root.codeExpired
                }
                loading: root.busy

                text: root.captchaStep ? qsTr("Continue") : (root.emailStep ? qsTr("Send code") : qsTr("Confirm"))

                onClicked: root.submit()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16

                visible: root.codeStep
                spacing: 16

                LinkButtonType {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1

                    enabled: !root.busy

                    text: qsTr("Change email")

                    onClicked: {
                        codeField.errorText = ""
                        root.step = "email"
                    }
                }

                LinkButtonType {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1

                    enabled: !root.busy && root.resendCooldownSec <= 0

                    text: root.resendCooldownSec > 0 ? qsTr("Resend in %1").arg(root.formattedTime(root.resendCooldownSec)) : qsTr("Resend code")

                    onClicked: root.resendCode()
                }
            }
        }
    }
}
