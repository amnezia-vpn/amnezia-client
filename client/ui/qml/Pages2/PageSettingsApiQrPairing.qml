import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property int qrImageIndex: 0

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
                defaultColor: "transparent"
                text: qsTr("Cancel receive")
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

            Image {
                id: qrImage
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                visible: PairingUiController.tvQrCodesCount > 0
                width: Math.min(220, parent.width - 32)
                height: width
                fillMode: Image.PreserveAspectFit
                source: PairingUiController.tvQrCodesCount > 0 ? PairingUiController.tvQrCodes[root.qrImageIndex] : ""

                MouseArea {
                    anchors.fill: parent
                    enabled: PairingUiController.tvQrCodesCount > 1
                    onClicked: {
                        root.qrImageIndex = (root.qrImageIndex + 1) % PairingUiController.tvQrCodesCount
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
                text: PairingUiController.phonePairingBusy ? qsTr("Sending…") : qsTr("Send from current subscription")
                enabled: !PairingUiController.tvPairingBusy && !PairingUiController.phonePairingBusy
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

        function onTvPairingConfigReceived() {
            PageController.showNotificationMessage(qsTr("Configuration received from gateway"))
        }

        function onPhonePairingSucceeded() {
            PageController.showNotificationMessage(qsTr("Configuration sent"))
        }
    }
}
