import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import QRCodeReader 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    ColumnLayout {
        anchors.fill: parent

        spacing: 0

        BackButtonType {
            Layout.topMargin: 20
        }

        ParagraphTextType {
            Layout.fillWidth: true

            text: qsTr("Point the camera at the QR code and hold for a couple of seconds.")
        }

        ProgressBarType {

        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            QRCodeReader {
               id: qrCodeReader
               Component.onCompleted: {
                   qrCodeReader.setCameraSize(Qt.rect(parent.x,
                                                      parent.y,
                                                      parent.width,
                                                      parent.height))
                   qrCodeReader.startReading()
               }
            }
        }
    }
}
