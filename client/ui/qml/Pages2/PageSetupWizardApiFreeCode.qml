import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    initialFocusItem: codeField.textField

    property string email: ""
    property int resendCooldownSec: 0
    property int codeTtlSec: 0
    property int failedAttempts: 0
    property bool locked: false
    property bool busy: false
    property bool resending: false

    readonly property int defaultResendCooldownSec: 60
    readonly property int defaultCodeTtlSec: 300
    readonly property int maxAttempts: 10
    readonly property int codeLength: 8

    function runBusy(action) {
        root.busy = true
        PageController.showBusyIndicator(true)
        Qt.callLater(function() {
            action()
            PageController.showBusyIndicator(false)
            root.busy = false
        })
    }

    function formattedTime(totalSeconds) {
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function startCodeSession(retryAfterSec) {
        codeField.textField.text = ""
        codeField.errorText = ""
        root.failedAttempts = 0
        root.locked = false
        root.codeTtlSec = root.defaultCodeTtlSec
        root.resendCooldownSec = retryAfterSec > 0 ? retryAfterSec : root.defaultResendCooldownSec
        codeField.textField.forceActiveFocus()
    }

    function submit() {
        if (root.locked || root.busy) {
            return
        }
        root.resending = false
        var value = codeField.textField.text.trim()
        if (value.length === 0) {
            codeField.errorText = qsTr("Enter the code from the email")
            return
        }
        root.runBusy(function() {
            SubscriptionUiController.confirmFreeEmailCode(value)
        })
    }

    function resend() {
        if (root.locked || root.busy || root.resendCooldownSec > 0) {
            return
        }
        root.resending = true
        root.runBusy(function() {
            SubscriptionUiController.resendFreeEmailCode()
        })
    }

    function lock(message) {
        root.locked = true
        codeField.errorText = message
    }

    function errorMessage(kind, retryAfterSec) {
        switch (kind) {
        case "captchaInvalid": return qsTr("CAPTCHA was incorrect. Please try again")
        case "captchaRetry": return qsTr("Please solve the new CAPTCHA to continue")
        case "noNetwork": return qsTr("No connection. Check your internet and try again")
        case "rateLimited": return retryAfterSec > 0 ? qsTr("Too many requests. Try again in %1 s").arg(retryAfterSec)
                                                     : qsTr("Too many requests. Try again later")
        default: return qsTr("Something went wrong. Try again later")
        }
    }

    Component.onCompleted: {
        root.email = SubscriptionUiController.freeAccountEmail()
        root.startCodeSession(SubscriptionUiController.freeAccountRetryAfterSec())
    }

    onCodeTtlSecChanged: {
        if (root.codeTtlSec <= 0 && !root.locked && codeField.errorText === "") {
            codeField.errorText = qsTr("The code has expired. Request a new one")
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.visible && (root.resendCooldownSec > 0 || root.codeTtlSec > 0)

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
        enabled: root.visible

        function onFreeEmailCodeSent(retryAfterSec) {
            root.resending = false
            root.startCodeSession(retryAfterSec)
        }

        function onFreeAccountLoginFailed(kind, retryAfterSec) {
            if (kind === "" || kind === "config") {
                root.resending = false
                return
            }

            if (root.resending) {
                root.resending = false
                if (kind === "rateLimited" && retryAfterSec > 0) {
                    root.resendCooldownSec = retryAfterSec
                }
                PageController.showErrorMessage(root.errorMessage(kind, retryAfterSec))
                return
            }

            switch (kind) {
            case "invalidCode":
                root.failedAttempts += 1
                if (root.failedAttempts >= root.maxAttempts) {
                    root.lock(qsTr("Too many attempts. Try again later"))
                } else {
                    codeField.errorText = qsTr("Incorrect code. Check it and try again")
                }
                break
            case "rateLimited":
                root.lock(qsTr("Too many attempts. Try again later"))
                break
            default:
                PageController.showErrorMessage(root.errorMessage(kind, retryAfterSec))
                break
            }
        }
    }

    FreeAccountCaptchaDialog {
        active: root.visible
    }

    FlickableType {
        id: flick

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: scrollColumn.implicitHeight + 16

        ColumnLayout {
            id: scrollColumn

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Check your email")
                description: qsTr("We sent a code to %1").arg(root.email)
            }

            TextFieldType {
                id: codeField

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: !root.locked

                headerText: qsTr("Code")
                textField.placeholderText: qsTr("Code from the email")
                textField.inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
                textField.maximumLength: root.codeLength
                textField.validator: RegularExpressionValidator { regularExpression: /^[0-9A-Za-z]{0,8}$/ }

                textField.onTextChanged: {
                    if (root.locked) {
                        return
                    }
                    if (codeField.errorText !== "") {
                        codeField.errorText = ""
                    }
                    if (codeField.textField.text.length === root.codeLength) {
                        root.submit()
                    }
                }

                textField.onAccepted: root.submit()
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            Layout.fillWidth: true

            enabled: !root.locked && !root.busy

            text: qsTr("Continue")

            onClicked: root.submit()
        }

        LinkButtonType {
            Layout.alignment: Qt.AlignHCenter

            enabled: !root.locked && !root.busy && root.resendCooldownSec <= 0
            colored: root.resendCooldownSec <= 0

            text: root.resendCooldownSec > 0 ? qsTr("Resend code in %1").arg(root.formattedTime(root.resendCooldownSec))
                                             : qsTr("Resend code")

            onClicked: root.resend()
        }
    }
}
