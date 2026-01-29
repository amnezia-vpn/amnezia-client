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

    property bool scanCompleted: false

    Item {
        id: cameraArea
        anchors.fill: parent
    }

    Loader {
        id: iosQrLoader
        anchors.fill: cameraArea
        active: Qt.platform.os === "ios"

        sourceComponent: Component {
            QRCodeReader {
                id: qrCodeReader

                function updateCameraSize() {
                    qrCodeReader.setCameraSize(Qt.rect(cameraArea.x,
                                                       cameraArea.y,
                                                       cameraArea.width,
                                                       cameraArea.height))
                }

                onCodeReaded: function(code) {
                    if (!code || code.length === 0) {
                        return
                    }

                    var obj = null
                    try {
                        obj = JSON.parse(code)
                    } catch (e) {
                        obj = null
                    }

                    if (!obj || !obj.gw || !(obj.uuid || obj.u)) {
                        return
                    }

                    var normalizedObj = { gw: obj.gw, uuid: (obj.uuid ? obj.uuid : obj.u) }
                    if (obj.name && obj.name.length > 0) {
                        normalizedObj.name = obj.name
                    }
                    var normalized = JSON.stringify(normalizedObj)
                    TransferController.setPendingQrCode(normalized)
                    qrCodeReader.stopReading()
                PageController.goToPage(PageEnum.PageSettingsApiAddDeviceConfirm)
            }

                Component.onCompleted: {
                    updateCameraSize()
                    qrCodeReader.startReading()
                }
                Component.onDestruction: qrCodeReader.stopReading()
                    }
                }
    }

    onWidthChanged: {
        if (iosQrLoader.item && iosQrLoader.item.updateCameraSize) {
            iosQrLoader.item.updateCameraSize()
        }
    }
    onHeightChanged: {
        if (iosQrLoader.item && iosQrLoader.item.updateCameraSize) {
            iosQrLoader.item.updateCameraSize()
        }
    }

    Connections {
        target: TransferController
        function onScannerShouldStop() {
            if (iosQrLoader.item && iosQrLoader.item.stopReading) {
                iosQrLoader.item.stopReading()
            }
        }
    }

    Connections {
        target: ImportController

        function onTransferQrDecoded(code) {
            if (!code || code.length === 0) {
                return
            }

            var obj = null
            try {
                obj = JSON.parse(code)
            } catch (e) {
                obj = null
            }
            if (obj && obj.gw && (obj.uuid || obj.u)) {
                var normalizedObj = { gw: obj.gw, uuid: (obj.uuid ? obj.uuid : obj.u) }
                if (obj.name && obj.name.length > 0) {
                    normalizedObj.name = obj.name
                }
                code = JSON.stringify(normalizedObj)
            }

            root.scanCompleted = true
            TransferController.setPendingQrCode(code)
            Qt.callLater(function() {
            PageController.goToPage(PageEnum.PageSettingsApiAddDeviceConfirm)
            })
        }

        function onQrDecodingFinished() {
            if (Qt.platform.os === "android" && !root.scanCompleted) {
                PageController.closePage()
            }
        }
    }

    Component.onCompleted: {
        TransferController.setPendingQrCode("")
        root.scanCompleted = false

        if (Qt.platform.os === "android") {
            ImportController.startDecodingQr()
        }
    }
}
