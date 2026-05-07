import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property int qrImageIndex: 0
    property int pairingSecondsLeft: 0

    function formatMmSs(totalSec) {
        if (totalSec <= 0) {
            return "0:00"
        }
        const m = Math.floor(totalSec / 60)
        const s = totalSec % 60
        return m + (s < 10 ? ":0" : ":") + s
    }

    function scrollPairingToBottom() {
        receiveScroll.contentY = Math.max(0, receiveScroll.contentHeight - receiveScroll.height)
    }

    Timer {
        id: scrollToBottomRetryTimer
        interval: 48
        repeat: true
        property int retries: 0
        onTriggered: {
            root.scrollPairingToBottom()
            retries++
            if (retries >= 12) {
                stop()
            }
        }
        onRunningChanged: {
            if (!running) {
                retries = 0
            }
        }
    }

    Timer {
        id: pairingCountdownTimer
        interval: 1000
        repeat: true
        running: PairingUiController.tvPairingBusy && PairingUiController.tvQrCodesCount > 0 && root.pairingSecondsLeft > 0
        onTriggered: {
            if (root.pairingSecondsLeft > 0) {
                root.pairingSecondsLeft--
            }
        }
    }

    Connections {
        target: root
        function onVisibleChanged() {
            if (!root.visible) {
                PairingUiController.cancelAllPairingActivity()
                scrollToBottomRetryTimer.stop()
                pairingCountdownTimer.stop()
                root.pairingSecondsLeft = 0
            }
        }
    }

    FlickableType {
        id: receiveScroll
        anchors.fill: parent
        contentHeight: layout.implicitHeight

        Behavior on contentY {
            NumberAnimation {
                duration: 320
                easing.type: Easing.OutCubic
            }
        }

        onContentHeightChanged: {
            if (PairingUiController.tvQrCodesCount > 0) {
                Qt.callLater(root.scrollPairingToBottom)
            }
        }

        ColumnLayout {
            id: layout
            width: root.width
            spacing: 8

            BackButtonType {
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                text: qsTr("Get Premium server from mobile")
                font.pixelSize: 28
                font.bold: true
                color: AmneziaStyle.color.paleGray
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                color: AmneziaStyle.color.goldenApricot
                text: qsTr("Amnezia Premium only. Someone who already has this subscription in Amnezia on a phone or tablet must send it to you; otherwise the session expires.")
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                text: qsTr("How to get the server from a mobile device")
                font.pixelSize: 18
                font.bold: true
                color: AmneziaStyle.color.mutedGray
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("On this device (TV, tablet, or second phone):\n1) In the “Start receiving” section, tap “Start and show QR” and leave this screen open until the transfer finishes or times out.\n\nOn the mobile device that already has Amnezia Premium:\n2) Open Amnezia VPN → Settings (gear).\n3) Select your Amnezia Premium API server in the list, then open its details screen.\n4) Choose “Transfer by QR (send)”.\n5) Scan the QR code shown on this device, or paste the session ID if you copy it from this screen.\n6) Tap “Send from current subscription” and wait. When the gateway completes pairing, this device receives the configuration and adds the server.")
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: PairingUiController.tvStatusMessage.length > 0
                text: PairingUiController.tvStatusMessage
                wrapMode: Text.Wrap
            }

            Item {
                id: qrBox
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.preferredHeight: PairingUiController.tvQrCodesCount > 0 ? width : 0
                visible: PairingUiController.tvQrCodesCount > 0

                Image {
                    id: qrImage
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    sourceSize: Qt.size(2048, 2048)
                    source: PairingUiController.tvQrCodesCount > 0 ? PairingUiController.tvQrCodes[root.qrImageIndex] : ""

                    MouseArea {
                        anchors.fill: parent
                        enabled: PairingUiController.tvQrCodesCount > 1
                        onClicked: {
                            root.qrImageIndex = (root.qrImageIndex + 1) % PairingUiController.tvQrCodesCount
                        }
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                visible: PairingUiController.tvPairingBusy && PairingUiController.tvQrCodesCount > 0
                color: AmneziaStyle.color.mutedGray
                text: qsTr("This QR code will refresh in %1. If the session expires, tap Start again for a new code.")
                      .arg(root.formatMmSs(root.pairingSecondsLeft))
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                text: qsTr("Start receiving")
                font.pixelSize: 18
                font.bold: true
                color: AmneziaStyle.color.mutedGray
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: PairingUiController.tvPairingBusy ? qsTr("Waiting…") : qsTr("Start and show QR")
                enabled: !PairingUiController.tvPairingBusy && !PairingUiController.phonePairingBusy
                clickedFunc: function() {
                    PairingUiController.startTvQrSession()
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Cancel receive")
                enabled: PairingUiController.tvPairingBusy
                clickedFunc: function() {
                    PairingUiController.cancelTvQrSession()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 24 + PageController.safeAreaBottomMargin
            }
        }
    }

    Connections {
        target: PairingUiController

        function onTvQrCodesChanged() {
            root.qrImageIndex = 0
            if (PairingUiController.tvQrCodesCount > 0) {
                root.pairingSecondsLeft = PairingUiController.tvPairingWaitWindowSeconds
                scrollToBottomRetryTimer.retries = 0
                scrollToBottomRetryTimer.start()
                Qt.callLater(function() {
                    root.scrollPairingToBottom()
                })
                Qt.callLater(function() {
                    Qt.callLater(function() {
                        root.scrollPairingToBottom()
                    })
                })
            }
        }

        function onTvSessionUuidChanged() {
            root.qrImageIndex = 0
        }

        function onTvPairingConfigReceived() {
            scrollToBottomRetryTimer.stop()
            root.pairingSecondsLeft = 0
            qrImage.source = ""
            PageController.showNotificationMessage(qsTr("Configuration received from gateway"))
            Qt.callLater(function() {
                PageController.closePage()
            })
        }
    }

}
