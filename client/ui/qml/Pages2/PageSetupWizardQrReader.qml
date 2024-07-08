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

        text: qsTr("Point the camera at the QR code and hold for a couple of seconds. ") + progressString
    }

    ProgressBarType {
        id: progressBar

        anchors.left: parent.left
        anchors.top: header.bottom
        anchors.right: parent.right

        anchors.leftMargin: 16
        anchors.rightMargin: 16
    }

    Rectangle {
        id: qrCodeRectange
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.top: progressBar.bottom

        anchors.topMargin: 34
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 34

        color: AmneziaStyle.color.transparent
        //radius: 16

        QRCodeReader {
           id: qrCodeReader

           onCodeReaded: function(code) {
               ImportController.parseQrCodeChunk(code)
               progressBar.value = ImportController.getQrCodeScanProgressBarValue()
               header.progressString = ImportController.getQrCodeScanProgressString()
           }

           Component.onCompleted: {
               qrCodeReader.setCameraSize(Qt.rect(qrCodeRectange.x,
                                                  qrCodeRectange.y,
                                                  qrCodeRectange.width,
                                                  qrCodeRectange.height))
               qrCodeReader.startReading()
           }
           Component.onDestruction: qrCodeReader.stopReading()
        }
    }
}
