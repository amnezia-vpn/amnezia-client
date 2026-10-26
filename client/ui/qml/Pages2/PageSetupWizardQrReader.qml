import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import QRCodeReader 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        NavigationBarType {
            Layout.fillWidth: true
        }

        H3TextType {
            id: headerText

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Point the camera at the QR code and hold for a couple of seconds. ")
        }

        ProgressBarType {
            id: progressBar

            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16
        }

        SmallTextType {
            id: progressText

            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.preferredHeight: lineHeight

            color: AmneziaStyle.color.textTertiary
            maximumLineCount: 1
            elide: Text.ElideRight
        }
    }

    Image {
        id: scanFrame

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: header.bottom
        anchors.topMargin: 46

        width: Math.min(parent.width - 92, 298)
        height: width

        source: "qrc:/images/controls/qr-scan-frame.svg"
        sourceSize: Qt.size(width, height)
    }

    Rectangle {
        id: qrCodeRectange

        anchors.fill: scanFrame
        anchors.margins: 3

        radius: 16
        color: AmneziaStyle.color.transparent

        QRCodeReader {
           id: qrCodeReader

           onCodeReaded: function(code) {
               ImportController.parseQrCodeChunk(code)
               progressBar.value = ImportController.getQrCodeScanProgressBarValue()
               progressText.text = ImportController.getQrCodeScanProgressString()
           }

           function start() {
               qrCodeReader.setCameraSize(Qt.rect(qrCodeRectange.x,
                                                  qrCodeRectange.y,
                                                  qrCodeRectange.width,
                                                  qrCodeRectange.height))
               qrCodeReader.startReading()
           }

           Component.onCompleted: Qt.callLater(qrCodeReader.start)
           Component.onDestruction: qrCodeReader.stopReading()
        }
    }
}
