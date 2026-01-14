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

    Rectangle {
        anchors.fill: parent
        color: AmneziaStyle.color.midnightBlack
        z: 0
    }

    MouseArea {
        anchors.fill: parent
        z: 0
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        preventStealing: true
        onPressed: mouse.accepted = true
        onReleased: mouse.accepted = true
        onClicked: mouse.accepted = true
        onWheel: wheel.accepted = true
    }

    ColumnLayout {
        z: 1
        anchors.fill: parent
        anchors.topMargin: 24
        spacing: 12

        BackButtonType {
            Layout.topMargin: 20
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            Layout.alignment: Qt.AlignLeft
        }

        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            ParagraphTextType {
                id: qrHeader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                text: qsTr("Scan the QR code with the phone where Amnezia Premium is active")
            }

            Rectangle {
                id: qrContainer
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: qrHeader.bottom
                anchors.topMargin: 8
                // anchors.horizontalCenter: parent.horizontalCenter  // УБРАТЬ
                anchors.bottom: bottomHint.top
                anchors.bottomMargin: 8

                color: AmneziaStyle.color.transparent
                border.color: AmneziaStyle.color.paleGray
                border.width: 1

                Image {
                    id: qrImage
                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height)
                    height: width
                    source: TransferController.qrCodeUrl
                    fillMode: Image.Stretch
                    smooth: false
                }
            }

            SmallTextType {
                id: bottomHint
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                text: qsTr("In the Amnezia app on your phone, tap «+» at the bottom → «QR code»")
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Connections {
        target: TransferController
        function onQrCodeUpdated() { qrImage.source = TransferController.qrCodeUrl }
        function onConfigApplied() {
            PageController.showNotificationMessage(qsTr("Configuration received and applied"))
        }
        function onWaitError(message) {
            PageController.showErrorMessage(message)
        }
    }

    Connections {
        target: ImportController
        function onImportErrorOccurred(errorCode, goToPageHome) {
            PageController.showErrorMessage(errorCode)
        }
    }

    Component.onCompleted: {
        TransferController.generateNewQrCode()
        TransferController.startWaitForConfig(ImportController)
    }

    Component.onDestruction: TransferController.stopWaitForConfig()
} 

/*CardWithIconsType {
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
}*/
