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

    ColumnLayout {
        z: 1
        anchors.fill: parent
        anchors.topMargin: 24
        spacing: 12

        BackButtonType {
            Layout.topMargin: 20
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.alignment: Qt.AlignLeft
        }

        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            ColumnLayout {
                id: qrContent
                anchors.fill: parent
                spacing: 16

                Item { Layout.fillHeight: true }

                SmallTextType {
                    id: topHint
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Scan this QR code with a phone that has an active\nAmnezia Premium subscription")
                }

                Rectangle {
                    id: qrFrame
                    Layout.alignment: Qt.AlignHCenter
                    property real maxByHeight: qrContent.height
                                              - topHint.implicitHeight
                                              - bottomHint.implicitHeight
                                              - (qrContent.spacing * 2)
                    property real qrSize: Math.max(180, Math.min(qrContent.width, Math.max(0, maxByHeight)))

                    Layout.preferredWidth: qrSize
                    Layout.preferredHeight: qrSize
                    radius: 16
                    color: "white"

                    Image {
                        id: qrImage
                        anchors.fill: parent
                        anchors.margins: 12
                        fillMode: Image.PreserveAspectFit
                        smooth: false
                        sourceSize: Qt.size(Math.round(width), Math.round(height))
                        source: TransferController.qrCodeUrl
                        visible: TransferController.qrCodeUrl !== ""
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: TransferController.qrCodeUrl === ""
                        visible: TransferController.qrCodeUrl === ""
                    }
                }

                SmallTextType {
                    id: bottomHint
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("AmneziaVPN → Amnezia Premium →\nPersonal Dashboard → Active Devices →\nAdd Device via QR Code")
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    Connections {
        target: TransferController
        function onConfigApplied() {
            PageController.showNotificationMessage(qsTr("Device has been added to subscription"))
            PageController.closePage()
            PageController.goToPageHome()
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
