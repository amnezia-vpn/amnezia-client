import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    property bool isSending: false

    function getAddedDeviceName() {
        try {
            var obj = JSON.parse(TransferController.pendingQrCode)
            if (obj && obj.name && obj.name.length > 0) {
                return obj.name
            }
        } catch (e) {}
        return qsTr("Device")
    }

    function getAvailableCount() {
        var max = ApiAccountInfoModel.data("maxDeviceCount")
        var active = ApiAccountInfoModel.data("activeDeviceCount")
        if (!max || max <= 0) max = 7
        if (!active || active < 0) active = 0
        var remain = max - active
        return remain > 0 ? remain : 0
    }

    ListViewType {
        id: listView

        anchors.fill: parent
        anchors.topMargin: 20

        header: ColumnLayout {
            width: listView.width

            BackButtonType {
                backButtonFunction: function() {
                    if (root.isSending) {
                        TransferController.stopWaitForConfig()
                    }
                    PageController.closePage()
                }
            }

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Add a new device to the subscription?")
                descriptionText: qsTr("Devices available with Amnezia Premium: (%1)").arg(getAvailableCount())
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                text: qsTr("Add Device")
                enabled: !root.isSending && root.getAvailableCount() > 0 && TransferController.pendingQrCode !== ""

                clickedFunc: function() {
                    if (TransferController.pendingQrCode !== "") {
                        root.isSending = true
                        TransferController.onTransferQrScanned(TransferController.pendingQrCode)
                    }
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Cancel")
                enabled: !root.isSending

                clickedFunc: function() {
                    PageController.closePage()
                }
            }
        }
    }

    Connections {
        target: TransferController

        function onPostStarted() {
            PageController.showNotificationMessage(qsTr("Sending configuration..."))
        }

        function onPostSucceeded() {
            root.isSending = false
            PageController.showNotificationMessage(qsTr("%1 has been added to your subscription").arg(root.getAddedDeviceName()))
            PageController.closePage()
            PageController.closePage()
        }

        function onPostFailed(message) {
            root.isSending = false
            PageController.showErrorMessage(message)
        }
    }
}


