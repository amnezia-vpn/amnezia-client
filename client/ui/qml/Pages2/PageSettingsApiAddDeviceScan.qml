import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    BackButtonType {
        id: backButton
        anchors.left: parent.left
        anchors.top: parent.top

        anchors.topMargin: 20
    }

    ParagraphTextType {
        id: header

        property string progressString

        anchors.left: parent.left
        anchors.top: backButton.bottom
        anchors.right: parent.right

        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: qsTr("Debug mode: paste payload JSON from the other device (or enter UUID). ") + progressString
    }

    ProgressBarType {
        id: progressBar

        anchors.left: parent.left
        anchors.top: header.bottom
        anchors.right: parent.right

        anchors.leftMargin: 16
        anchors.rightMargin: 16
    }

    // Debug: manual payload/UUID input (temporary instead of camera scanning)
    ColumnLayout {
        id: manualInput
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: progressBar.bottom
        anchors.bottom: parent.bottom

        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 24
        anchors.bottomMargin: 24
        spacing: 12

        TextFieldWithHeaderType {
            id: payloadField
            Layout.fillWidth: true

            headerText: qsTr("Payload JSON")
            textField.placeholderText: qsTr("Paste JSON like {\"gw\":\"...\",\"uuid\":\"...\"}")
        }

        TextFieldWithHeaderType {
            id: uuidField
            Layout.fillWidth: true

            headerText: qsTr("UUID")
            textField.placeholderText: qsTr("Enter UUID")
        }

        BasicButtonType {
            id: usePayloadButton
            Layout.fillWidth: true
            text: qsTr("Next (use payload)")
            enabled: payloadField.textField.text.length > 0

            clickedFunc: function() {
                TransferController.setPendingQrCode(payloadField.textField.text)
                PageController.goToPage(PageEnum.PageSettingsApiAddDeviceConfirm)
            }
        }

        BasicButtonType {
            id: buildFromUuidButton
            Layout.fillWidth: true
            defaultColor: AmneziaStyle.color.transparent
            hoveredColor: AmneziaStyle.color.translucentWhite
            pressedColor: AmneziaStyle.color.sheerWhite
            textColor: AmneziaStyle.color.paleGray
            borderColor: AmneziaStyle.color.paleGray
            borderWidth: 1

            text: qsTr("Next (build from UUID)")
            enabled: uuidField.textField.text.length > 0

            clickedFunc: function() {
                var gw = ""
                if (typeof SettingsController !== "undefined") {
                    if (SettingsController.gatewayEndpoint !== undefined) {
                        gw = SettingsController.gatewayEndpoint
                    } else if (SettingsController.getGatewayEndpoint) {
                        gw = SettingsController.getGatewayEndpoint()
                    }
                }
                if (!gw || gw.length === 0) {
                    PageController.showErrorMessage(qsTr("Gateway endpoint is empty"))
                    return
                }
                if (!gw.endsWith("/")) {
                    gw = gw + "/"
                }

                var payload = JSON.stringify({ gw: gw, uuid: uuidField.textField.text })
                TransferController.setPendingQrCode(payload)
                PageController.goToPage(PageEnum.PageSettingsApiAddDeviceConfirm)
            }
        }
    }

    /*Rectangle {
        id: qrCodeRactangle
        visible: false
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.top: progressBar.bottom

        anchors.topMargin: 34
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 34

        color: AmneziaStyle.color.transparent

        QRCodeReader {
            id: qrCodeReader

            onCodeReaded: function(code) {
                // scanning disabled
                return
            }

            Component.onCompleted: { }

            Component.onDestruction: qrCodeReader.stopReading()
        }
    }*/

    Component.onCompleted: { }

    /*Connections {
        target: TransferController
        function onScannerShouldStop() {
            if (qrCodeReader && qrCodeReader.stopReading) {
                qrCodeReader.stopReading()
            }
            if (typeof ImportController !== "undefined"
                    && ImportController.stopDecodingQr) {
                ImportController.stopDecodingQr()
            }
        }
    }

    Connections {
        target: ImportController

        function onTransferQrDecoded(code) {
            if (!code || code.length === 0) {
                console.log("ImportController.onTransferQrDecoded: empty QR code payload")
                if (typeof PageController !== "undefined" && PageController.showErrorMessage) {
                    PageController.showErrorMessage(qsTr("QR code not recognized. Please try again."));
                }
                return
            }

            TransferController.setPendingQrCode(code)
            PageController.goToPage(PageEnum.PageSettingsApiAddDeviceConfirm)
        }
    }*/
}
