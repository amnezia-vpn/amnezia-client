import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0
import "../Controls2"
import "../Components"
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
                text: qsTr("Debug mode: copy the payload to the other device and send it there")
            }

            ColumnLayout {
                id: debugPayload
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: qrHeader.bottom
                anchors.topMargin: 12
                anchors.bottom: bottomHint.top
                anchors.bottomMargin: 12
                spacing: 8

                TextArea {
                    id: payloadArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    readOnly: true
                    wrapMode: TextArea.WrapAnywhere
                    selectByMouse: true
                    text: TransferController.currentPayload
                    placeholderText: qsTr("Payload will appear here after generating")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    BasicButtonType {
                        Layout.fillWidth: true
                        text: qsTr("Regenerate")
                        clickedFunc: function() {
                            TransferController.generateNewQrCode()
                            TransferController.startWaitForConfig(ImportController)
                        }
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        defaultColor: AmneziaStyle.color.transparent
                        hoveredColor: AmneziaStyle.color.translucentWhite
                        pressedColor: AmneziaStyle.color.sheerWhite
                        textColor: AmneziaStyle.color.paleGray
                        borderColor: AmneziaStyle.color.paleGray
                        borderWidth: 1
                        text: qsTr("Restart wait")
                        clickedFunc: function() {
                            TransferController.startWaitForConfig(ImportController)
                        }
                    }
                }
            }

            SmallTextType {
                id: bottomHint
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                text: qsTr("Copy the JSON payload above, paste it on the other device, and confirm sending there")
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Connections {
        target: TransferController
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
