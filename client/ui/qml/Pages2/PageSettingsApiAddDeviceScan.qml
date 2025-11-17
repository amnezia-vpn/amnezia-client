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

    // Единая логика при входе на страницу
    Component.onCompleted: {
        // Страница имеет смысл только на мобилках
        if (!GC.isMobile()) {
            PageController.closePage()
            return
        }

        // Проверяем наличие камеры
        if (!SettingsController.isCameraPresent()) {
            PageController.showErrorMessage(qsTr("Camera is not available on this device"))
            PageController.closePage()
            return
        }

        // Android: запускаем нативный сканер камеры
        if (Qt.platform.os === "android"
                && typeof ImportController !== "undefined"
                && ImportController.startDecodingQr) {
            ImportController.startDecodingQr()
        }
    }

    ListViewType {
        id: listView
        visible: GC.isMobile()

        anchors.fill: parent
        anchors.topMargin: 20

        header: ColumnLayout {
            width: listView.width

            BackButtonType {}

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Point the camera at the QR code and hold for a couple of seconds.")
            }
        }

        footer: Item { height: 0 }

        model: 1

        delegate: Item {
            width: listView.width
            height: listView.height - 100

            Rectangle {
                id: cameraArea
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.bottomMargin: 16
                anchors.topMargin: 16
                visible: Qt.platform.os === "ios"  // На Android используется нативная камера

                color: AmneziaStyle.color.transparent
                border.color: AmneziaStyle.color.paleGray
                border.width: 1

                // ВАЖНО: ридер виден на всех мобильных платформах
                QRCodeReader {
                    id: qrReader

                    onCodeReaded: function (code) {
                        if (!code || code.length === 0)
                            return

                        qrReader.stopReading()
                        TransferController.setPendingQrCode(code)
                        PageController.goToPage(PageEnum.PageSettingsApiAddDeviceConfirm)
                    }

                    Component.onCompleted: {
                        qrReader.setCameraSize(
                                    Qt.rect(cameraArea.x,
                                            cameraArea.y,
                                            cameraArea.width,
                                            cameraArea.height))
                        qrReader.startReading()
                    }

                    Component.onDestruction: {
                        qrReader.stopReading()
                    }
                }

                Connections {
                    target: TransferController
                    function onScannerShouldStop() {
                        qrReader.stopReading()
                    }
                }
            }
        }
    }
}
