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

            BackButtonType {}

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Share VPN with a new device?")
                descriptionText: qsTr("Your Amnezia Premium subscription can connect %1 more devices").arg(getAvailableCount())
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                text: qsTr("Yes, share")

                clickedFunc: function() {
                    if (TransferController.pendingQrCode !== "") {
                        TransferController.onTransferQrScanned(TransferController.pendingQrCode)
                    }
                    PageController.closePage()
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

                clickedFunc: function() {
                    PageController.closePage()
                }
            }
        }
    }
}


