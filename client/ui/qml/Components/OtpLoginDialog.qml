pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

Popup {
    id: root

    property string serverId: ""
    property string otpCode: ""
    property int secondsLeft: 0
    property string errorKind: ""
    property int retryCooldownSec: 0

    // loading | code | expired | error
    property string dialogState: "loading"

    readonly property string accountUrl: "https://cp.amnezia.org"
    readonly property bool showsCode: dialogState === "loading" || dialogState === "code"

    function requestCode(serverId) {
        root.serverId = serverId
        root.otpCode = ""
        root.errorKind = ""
        root.dialogState = "loading"
        countdown.stop()
        statusPoll.stop()
        if (!root.opened) {
            root.open()
        }
        Qt.callLater(function() {
            SubscriptionUiController.otpLogin(root.serverId)
        })
    }

    function formattedCode() {
        var half = Math.ceil(root.otpCode.length / 2)
        return root.otpCode.length > 4
                ? root.otpCode.substring(0, half) + " " + root.otpCode.substring(half)
                : root.otpCode
    }

    function formattedTime() {
        var minutes = Math.floor(root.secondsLeft / 60)
        var seconds = root.secondsLeft % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function titleText() {
        if (root.dialogState === "expired") {
            return qsTr("Code expired")
        }
        if (root.dialogState === "error") {
            switch (root.errorKind) {
            case "noNetwork": return qsTr("No connection")
            case "purchaseNotFound": return qsTr("Purchase hasn't reached us yet")
            case "rateLimited": return qsTr("Service is busy")
            case "receiptRejected":
            case "receiptRejectedAgain": return qsTr("Couldn't verify the purchase")
            default: return qsTr("Couldn't get a code")
            }
        }
        return qsTr("Code for your account")
    }

    function subtitleText() {
        if (root.dialogState === "expired") {
            return qsTr("Request a new code")
        }
        if (root.dialogState === "error") {
            switch (root.errorKind) {
            case "noNetwork": return qsTr("Check your connection and retry")
            case "purchaseNotFound": return qsTr("This happens right after payment or a plan change. Retry in a minute")
            case "receiptRejected": return qsTr("Try again")
            case "receiptRejectedAgain": return qsTr("Contact support and we'll help you sort it out")
            default: return qsTr("Try again later")
            }
        }
        return qsTr("Enter it at cp.amnezia.org to confirm the subscription is yours")
    }

    function primaryButtonText() {
        if (root.dialogState === "expired") {
            return qsTr("Get a new code")
        }
        if (root.dialogState === "error") {
            return root.errorKind === "receiptRejectedAgain" ? qsTr("Contact support") : qsTr("Retry")
        }
        return qsTr("Open account")
    }

    function primaryButtonEnabled() {
        if (root.dialogState === "loading") {
            return false
        }
        if (root.dialogState === "error" && root.errorKind === "rateLimited") {
            return root.retryCooldownSec <= 0
        }
        return true
    }

    function primaryButtonClicked() {
        if (root.dialogState === "code") {
            Qt.openUrlExternally(root.accountUrl)
        } else if (root.dialogState === "error" && root.errorKind === "receiptRejectedAgain") {
            root.close()
            PageController.goToPage(PageEnum.PageSettingsApiSupport)
        } else {
            root.requestCode(root.serverId)
        }
    }

    parent: Overlay.overlay

    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(parent.width - 32, 400)

    padding: 0

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.bgOverlay
    }

    background: Rectangle {
        color: AmneziaStyle.color.bgSurfaceAlt
        radius: 16
    }

    onClosed: {
        countdown.stop()
        statusPoll.stop()
        retryCooldown.stop()
    }

    Timer {
        id: countdown

        interval: 1000
        repeat: true

        onTriggered: {
            if (root.secondsLeft > 0) {
                root.secondsLeft -= 1
            } else {
                stop()
                statusPoll.stop()
                root.dialogState = "expired"
            }
        }
    }

    Timer {
        id: statusPoll

        interval: 3000
        repeat: true

        onTriggered: {
            SubscriptionUiController.checkOtpStatus()
        }
    }

    Timer {
        id: retryCooldown

        interval: 1000
        repeat: true

        onTriggered: {
            if (root.retryCooldownSec > 0) {
                root.retryCooldownSec -= 1
            } else {
                stop()
            }
        }
    }

    Connections {
        target: SubscriptionUiController

        function onOtpCodeReceived(code, expiresInSec) {
            if (!root.opened) {
                return
            }
            root.otpCode = code
            root.secondsLeft = expiresInSec
            root.dialogState = "code"
            countdown.restart()
            statusPoll.restart()
        }

        function onOtpLoginFailed(kind) {
            if (!root.opened) {
                return
            }
            if (kind === "") {
                root.close()
                return
            }
            root.errorKind = kind
            root.dialogState = "error"
            if (kind === "rateLimited") {
                root.retryCooldownSec = 30
                retryCooldown.restart()
            }
        }

        function onOtpConfirmed() {
            countdown.stop()
            statusPoll.stop()
            root.close()
            PageController.showNotificationMessage(qsTr("Code accepted"))
        }

        function onOtpExpired() {
            root.secondsLeft = 0
            countdown.stop()
            statusPoll.stop()
            root.dialogState = "expired"
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        H3TextType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: root.titleText()
        }

        BodyTextType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            color: AmneziaStyle.color.textTertiary
            text: root.subtitleText()
        }

        Rectangle {
            id: codeBox

            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            implicitHeight: 96

            visible: root.showsCode
            radius: 16
            color: AmneziaStyle.color.bgBase

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.6
                height: 48
                radius: 12

                visible: root.dialogState === "loading"
                color: AmneziaStyle.color.bgSurface
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 4

                visible: root.dialogState === "code"

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    H1TextType {
                        anchors.centerIn: parent

                        horizontalAlignment: Text.AlignHCenter
                        font.letterSpacing: 2
                        text: root.formattedCode()
                    }

                    IconButtonType {
                        id: copyButton

                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter

                        size: IconButtonType.Small

                        icon.source: "qrc:/images/controls/copy.svg"

                        onClicked: {
                            GC.copyToClipBoard(root.otpCode)
                            PageController.showNotificationMessage(qsTr("Code copied"))
                        }
                    }
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 12

                    horizontalAlignment: Text.AlignHCenter
                    color: AmneziaStyle.color.textTertiary
                    text: qsTr("%1 left").arg(root.formattedTime())
                }
            }
        }

        CaptionTextType {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            visible: root.showsCode
            horizontalAlignment: Text.AlignHCenter
            color: AmneziaStyle.color.textTertiary
            text: qsTr("Don't share this code with anyone, even support")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.bottomMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            DialogButtonType {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                text: qsTr("Close")

                onClicked: root.close()
            }

            DialogButtonType {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                enabled: root.primaryButtonEnabled()

                text: root.primaryButtonText()
                emphasized: true
                textColor: AmneziaStyle.color.textAccent

                onClicked: root.primaryButtonClicked()
            }
        }
    }
}
