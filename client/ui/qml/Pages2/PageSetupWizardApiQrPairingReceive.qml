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

    readonly property int qrRefreshIntervalMs: Math.max(5000, PairingUiController.tvPairingWaitWindowSeconds * 1000)

    function scrollPairingToBottom() {
        receiveScroll.contentY = Math.max(0, receiveScroll.contentHeight - receiveScroll.height)
    }

    function beginReceiveFlow() {
        PairingUiController.startTvQrSession()
        qrRotationTimer.restart()
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
        id: qrRotationTimer
        interval: root.qrRefreshIntervalMs
        repeat: true
        running: root.visible
        onTriggered: {
            PairingUiController.cancelTvQrSession()
            PairingUiController.startTvQrSession()
        }
    }

    Connections {
        target: root
        function onVisibleChanged() {
            if (!root.visible) {
                PairingUiController.cancelAllPairingActivity()
                scrollToBottomRetryTimer.stop()
                qrRotationTimer.stop()
            } else {
                Qt.callLater(root.beginReceiveFlow)
            }
        }
    }

    Component.onCompleted: {
        if (root.visible) {
            Qt.callLater(root.beginReceiveFlow)
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
            spacing: 12

            BackButtonType {
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Scan this QR code with a phone that has an active Amnezia Premium subscription")
                font.pixelSize: 17
                font.bold: false
                color: AmneziaStyle.color.paleGray
                wrapMode: Text.Wrap
            }

            Item {
                id: qrBox
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.topMargin: 16
                // Avoid width*0.92 before first layout (width can be 0 → zero height → no QR).
                Layout.preferredHeight: PairingUiController.tvQrCodesCount > 0 ? Math.max(200, layout.width - 48) : 0
                visible: PairingUiController.tvQrCodesCount > 0

                Rectangle {
                    anchors.fill: parent
                    radius: 20
                    color: "#FFFFFF"

                    Image {
                        id: qrImage
                        anchors.fill: parent
                        anchors.margins: 20
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(2048, 2048)
                        source: PairingUiController.tvQrCodesCount > 0 ? PairingUiController.tvQrCodes[0] : ""
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                horizontalAlignment: Text.AlignHCenter
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 13
                text: qsTr("AmneziaVPN → Amnezia Premium →\nPersonal Dashboard → Active Devices →\nAdd Device via QR Code")
                wrapMode: Text.Wrap
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
            if (PairingUiController.tvQrCodesCount > 0) {
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

        function onTvPairingConfigReceived() {
            scrollToBottomRetryTimer.stop()
            qrRotationTimer.stop()
            qrImage.source = ""
            PageController.showNotificationMessage(qsTr("Configuration received from gateway"))
            Qt.callLater(function() {
                PageController.closePage()
            })
        }
    }
}
