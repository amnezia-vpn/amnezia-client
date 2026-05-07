import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QRCodeReader 1.0
import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property int qrImageIndex: 0
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
                text: qsTr("QR pairing")
                font.pixelSize: 28
                font.bold: true
                color: AmneziaStyle.color.paleGray
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Experimental: transfer API configuration to another device via gateway. Use “Receive” on the device that shows the QR code, and “Send” on the premium device.")
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                text: qsTr("Receive configuration (TV / second device)")
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
                // Do not use defaultColor: transparent here: when enabled, BasicButtonType paints that
                // as the idle background, so midnightBlack label sits on the page — invisible until hover.
                enabled: PairingUiController.tvPairingBusy
                clickedFunc: function() {
                    PairingUiController.cancelTvQrSession()
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: PairingUiController.tvStatusMessage.length > 0
                text: PairingUiController.tvStatusMessage
                wrapMode: Text.Wrap
            }

            // SVG QR from qrCodeUtils has a tiny viewBox (~45px); without a sized container + sourceSize it stays small.
            Item {
                id: qrBox
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                implicitHeight: width
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

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                text: qsTr("Send configuration (premium device)")
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
                textField.placeholderText: qsTr("Paste UUID from TV QR")
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

                // QRCodeReader is a QObject (not Item): no anchors; preview rect via setCameraSize like PageSetupWizardQrReader.
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

        function onTvQrCodesChanged() {
            root.qrImageIndex = 0
        }

        function onTvSessionUuidChanged() {
            root.qrImageIndex = 0
            uuidField.textField.text = PairingUiController.tvSessionUuid
        }

        function onTvPairingConfigReceived() {
            PageController.showNotificationMessage(qsTr("Configuration received from gateway"))
        }

        function onPhonePairingSucceeded() {
            PageController.showNotificationMessage(qsTr("Configuration sent"))
        }

        function onPairingUuidFromScan(uuid) {
            uuidField.textField.text = uuid
        }
    }
}
