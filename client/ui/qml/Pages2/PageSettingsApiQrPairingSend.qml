import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QRCodeReader 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool pairingCameraOpen: false

    Connections {
        target: root
        function onVisibleChanged() {
            if (!root.visible) {
                pairingQrReader.stopReading()
                root.pairingCameraOpen = false
                PairingUiController.cancelAllPairingActivity()
            }
        }
    }

    FlickableType {
        anchors.fill: parent
        contentHeight: layout.implicitHeight

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
                text: qsTr("Transfer subscription (QR)")
                font.pixelSize: 28
                font.bold: true
                color: AmneziaStyle.color.paleGray
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Scan the session QR shown on the receiving device, then send this server’s Amnezia Premium configuration through the gateway.")
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                text: qsTr("Send from this subscription")
                font.pixelSize: 18
                font.bold: true
                color: AmneziaStyle.color.mutedGray
            }

            TextFieldWithHeaderType {
                id: uuidField
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("QR session UUID")
                textField.placeholderText: qsTr("Paste UUID from the other device’s QR")
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: Qt.platform.os === "android" || Qt.platform.os === "ios"
                text: {
                    if (Qt.platform.os === "ios" && root.pairingCameraOpen) {
                        return qsTr("Hide camera")
                    }
                    return qsTr("Scan QR code")
                }
                enabled: !PairingUiController.phonePairingBusy
                clickedFunc: function() {
                    if (Qt.platform.os === "android") {
                        PairingUiController.openPairingQrScanner()
                    } else {
                        root.pairingCameraOpen = !root.pairingCameraOpen
                    }
                }
            }

            Item {
                id: cameraSlot
                Layout.fillWidth: true
                Layout.preferredHeight: (root.pairingCameraOpen && Qt.platform.os === "ios") ? 220 : 0
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: Layout.preferredHeight > 0
                clip: true

                QRCodeReader {
                    id: pairingQrReader

                    onCodeReaded: function(code) {
                        if (PairingUiController.applyScannedTextAsPairingUuid(code)) {
                            pairingQrReader.stopReading()
                            root.pairingCameraOpen = false
                            PageController.showNotificationMessage(qsTr("Session ID filled from QR"))
                        }
                    }
                }

                onVisibleChanged: {
                    if (!visible) {
                        pairingQrReader.stopReading()
                        return
                    }
                    if (Qt.platform.os === "ios") {
                        Qt.callLater(function() {
                            var p = cameraSlot.mapToItem(root, 0, 0)
                            pairingQrReader.setCameraSize(Qt.rect(p.x, p.y, cameraSlot.width, cameraSlot.height))
                            pairingQrReader.startReading()
                        })
                    }
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: PairingUiController.phonePairingBusy ? qsTr("Sending…") : qsTr("Send from current subscription")
                enabled: !PairingUiController.phonePairingBusy
                clickedFunc: function() {
                    PairingUiController.submitPhonePairing(uuidField.textField.text, ServersUiController.getProcessedServerIndex())
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24 + PageController.safeAreaBottomMargin
                visible: PairingUiController.phoneStatusMessage.length > 0
                text: PairingUiController.phoneStatusMessage
                wrapMode: Text.Wrap
            }
        }
    }

    Connections {
        target: PairingUiController

        function onPhonePairingSucceeded() {
            root.pairingCameraOpen = false
            pairingQrReader.stopReading()
            PageController.showNotificationMessage(qsTr("Configuration sent"))
            Qt.callLater(function() {
                PageController.closePage()
            })
        }

        function onPairingUuidFromScan(uuid) {
            uuidField.textField.text = uuid
        }
    }
}
