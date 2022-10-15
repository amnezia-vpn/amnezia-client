import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0

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
}
