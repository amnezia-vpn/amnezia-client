import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import QRCodeReader 1.0

import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.QrDecoderIos
    logic: QrDecoderLogic

    onDeactivated: {
        console.debug("Stopping QR decoder")
        loader.sourceComponent = undefined
    }

    BackButton {
    }
    Caption {
        id: caption
        text: qsTr("Import configuration")
    }

    Connections {
        target: Qt.platform.os == "ios" ? QrDecoderLogic : null
        function onStartDecode() {
            console.debug("Starting QR decoder")
            loader.sourceComponent = component
        }
        function onStopDecode() {
            console.debug("Stopping QR decoder")
            loader.sourceComponent = undefined
        }
    }

    Loader {
        id: loader

        anchors.top: caption.bottom
        anchors.bottom: progressColumn.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Column{
    height: 40
    id: progressColumn
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
        ProgressBar {
            id: progress
            anchors.left: parent.left
            anchors.right: parent.right
            value: QrDecoderLogic.totalChunksCount === 0? 0 : (QrDecoderLogic.receivedChunksCount/QrDecoderLogic.totalChunksCount)
        }
        Text {
            id: chunksCount
            text: "Progress: " + QrDecoderLogic.receivedChunksCount +"/"+QrDecoderLogic.totalChunksCount
        }
    }

    Component {
        id: component

        Item {
            anchors.fill: parent

            QRCodeReader {
                id: qrCodeReader

                onCodeReaded: {
                    QrDecoderLogic.onDetectedQrCode(code)
                }

                Component.onCompleted: {
                    qrCodeReader.setCameraSize(Qt.rect(loader.x,
                                                       loader.y,
                                                       loader.width,
                                                       loader.height))
                    qrCodeReader.startReading()
                }
                Component.onDestruction: qrCodeReader.stopReading()
            }

        }

    }


}
