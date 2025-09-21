import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0
import "../Controls2"
import "../Components"
import QRCodeReader 1.0
import "../Controls2/TextTypes"

PageType {
    id: root
    objectName: "PageTransferConfigViaQR"

    property bool showReceiveSection: false
    property bool showScanSection: false
    property bool isPosting: false
    property string postStatusText: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 24
        spacing: 12

        BackButtonType {
            Layout.topMargin: 20
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            Layout.alignment: Qt.AlignLeft
        }

        HeaderTypeWithButton {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            headerText: qsTr("Transfer configuration")
            actionButtonImage: (showReceiveSection || showScanSection) ? "qrc:/images/controls/close.svg" : ""
            actionButtonFunction: function() {
                showReceiveSection = false
                showScanSection = false
            }
        }

        CardWithIconsType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8

            headerText: qsTr("Get connection via QR")
            bodyText: qsTr("Show a QR code that another device can scan to transfer your config here")
            leftImageSource: "qrc:/images/controls/qr-code.svg"
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                showScanSection = false
                showReceiveSection = true
                TransferController.generateNewQrCode()
                TransferController.startWaitForConfig(ImportController)
            }
        }

        CardWithIconsType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8

            headerText: qsTr("Send connection via QR")
            bodyText: qsTr("Scan a QR code from another device to send your config to it")
            leftImageSource: "qrc:/images/controls/scan-line.svg"
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                showReceiveSection = false
                showScanSection = true
                isPosting = false
                postStatusText = ""
            }
        }

        // Content area fills the rest of the page
        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            // Receive section (shows generated QR image)
            Item {
                id: receiveSection
                anchors.fill: parent
                visible: showReceiveSection

                ParagraphTextType {
                    id: qrHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    text: qsTr("Scan this QR code with your device to receive the configuration")
                }

                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    spacing: 12

                    BusyIndicator { running: true; visible: true; Layout.alignment: Qt.AlignVCenter }
                    ParagraphTextType { text: qsTr("Waiting..."); Layout.fillWidth: true }

                    BasicButtonType {
                        text: qsTr("Cancel")
                        Layout.preferredWidth: 120
                        clickedFunc: function() { TransferController.stopWaitForConfig(); showReceiveSection = false }
                    }
                }

                Rectangle {
                    id: qrContainer
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: qrHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 8
                    anchors.bottomMargin: 56
                    color: AmneziaStyle.color.transparent
                    border.color: AmneziaStyle.color.paleGray
                    border.width: 1

                    Image {
                        id: qrImage
                        anchors.fill: parent
                        source: TransferController.qrCodeUrl
                        fillMode: Image.PreserveAspectFit
                        smooth: false
                    }
                }
            }

            // Scan section (embedded QR scanner)
            Item {
                id: scanSection
                anchors.fill: parent
                visible: showScanSection

                ParagraphTextType {
                    id: scanHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    text: isPosting ? qsTr("Sending...") : (postStatusText !== "" ? postStatusText : qsTr("Point the camera at the QR code on the other device and hold for a couple of seconds"))
                }

                Rectangle {
                    id: scannerRect
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: scanHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 8
                    color: AmneziaStyle.color.transparent
                    border.color: AmneziaStyle.color.paleGray
                    border.width: 1

                    QRCodeReader {
                        id: qrReader

                        onCodeReaded: function(code) {
                            if (!isPosting) {
                                isPosting = true
                                postStatusText = ""
                                TransferController.onTransferQrScanned(code)
                            }
                        }

                        Component.onCompleted: {
                            qrReader.setCameraSize(Qt.rect(scannerRect.x,
                                                           scannerRect.y,
                                                           scannerRect.width,
                                                           scannerRect.height))
                            qrReader.startReading()
                        }
                        Component.onDestruction: qrReader.stopReading()
                    }
                }
            }
        }
    }

    Connections {
        target: TransferController
        function onQrCodeUpdated() {
            qrImage.source = TransferController.qrCodeUrl
        }
        function onScannerShouldStop() {
            showScanSection = false
            isPosting = false
            postStatusText = qsTr("Sent successfully")
        }
        function onConfigApplied() {
            showReceiveSection = false
            PageController.showNotificationMessage(qsTr("Configuration received and applied"))
        }
        function onWaitError(message) {
            PageController.showErrorMessage(message)
        }
        function onPostStarted() {
            isPosting = true
            postStatusText = ""
        }
        function onPostSucceeded() {
            isPosting = false
            postStatusText = qsTr("Sent successfully")
        }
        function onPostFailed(message) {
            isPosting = false
            postStatusText = message
        }
    }

    Connections {
        target: ImportController
        function onImportErrorOccurred(errorCode, goToPageHome) {
            PageController.showErrorMessage(errorCode)
        }
    }

    Component.onDestruction: {
        TransferController.stopWaitForConfig()
    }
} 