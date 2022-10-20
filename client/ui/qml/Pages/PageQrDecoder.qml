import QtQuick
import QtQuick.Controls
import QtMultimedia
import PageEnum 1.0
import QZXing 3.2

import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.QrDecoder
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
        target: Qt.platform.os != "ios" ? QrDecoderLogic : nil
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
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Component {
        id: component

        Item {
            anchors.fill: parent

            Camera
            {
                id:camera
                focus {
                    focusMode: CameraFocus.FocusContinuous
                    focusPointMode: CameraFocus.FocusPointAuto
                }
            }

            VideoOutput
            {
                id: videoOutput
                source: camera
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                autoOrientation: true
                fillMode: VideoOutput.PreserveAspectFit
                filters: [ zxingFilter ]


                Rectangle {
                    color: "black"
                    opacity: 0.5
                    width: videoOutput.contentRect.width * 0.15
                    height: videoOutput.contentRect.height
                    x: (videoOutput.width - videoOutput.contentRect.width)/2
                    anchors.verticalCenter: videoOutput.verticalCenter
                }

                Rectangle {
                    color: "black"
                    opacity: 0.5
                    width: videoOutput.contentRect.width * 0.15
                    height: videoOutput.contentRect.height
                    x: videoOutput.width/2 + videoOutput.contentRect.width/2 - videoOutput.contentRect.width * 0.15
                    anchors.verticalCenter: videoOutput.verticalCenter
                }

                Rectangle {
                    color: "black"
                    opacity: 0.5
                    width: videoOutput.contentRect.width * 0.7
                    height: videoOutput.contentRect.height * 0.15
                    x: (videoOutput.width - videoOutput.contentRect.width)/2 + videoOutput.contentRect.width * 0.15
                    y: (videoOutput.height - videoOutput.contentRect.height)/2
                }

                Rectangle {
                    color: "black"
                    opacity: 0.5
                    width: videoOutput.contentRect.width * 0.7
                    height: videoOutput.contentRect.height * 0.15
                    x: (videoOutput.width - videoOutput.contentRect.width)/2 + videoOutput.contentRect.width * 0.15
                    y: videoOutput.height/2 + videoOutput.contentRect.height/2 - videoOutput.contentRect.height * 0.15
                }

                LabelType {
                    width: parent.width
                    text: qsTr("Decoded QR chunks " + QrDecoderLogic.receivedChunksCount + "/" + QrDecoderLogic.totalChunksCount)
                    horizontalAlignment: Text.AlignLeft
                    visible: QrDecoderLogic.totalChunksCount > 0
                    anchors.horizontalCenter: videoOutput.horizontalCenter
                    y: videoOutput.height/2 + videoOutput.contentRect.height/2
                }
            }

            QZXingFilter
            {
                id: zxingFilter
                orientation: videoOutput.orientation
                captureRect: {
                    // setup bindings
                    videoOutput.contentRect;
                    videoOutput.sourceRect;
                    return videoOutput.mapRectToSource(videoOutput.mapNormalizedRectToItem(Qt.rect(
                        0.15, 0.15, 0.7, 0.7 //0, 0, 1.0, 1.0
                    )));
                }

                decoder {
                    enabledDecoders: QZXing.DecoderFormat_QR_CODE

                    onTagFound: {
                        QrDecoderLogic.onDetectedQrCode(tag)
                    }

                    tryHarder: true
                }

                property int framesDecoded: 0
                property real timePerFrameDecode: 0

                onDecodingFinished:
                {
                    timePerFrameDecode = (decodeTime + framesDecoded * timePerFrameDecode) / (framesDecoded + 1);
                    framesDecoded++;
                    if(succeeded)
                        console.log("frame finished: " + succeeded, decodeTime, timePerFrameDecode, framesDecoded);
                }
            }


        }

    }


}
