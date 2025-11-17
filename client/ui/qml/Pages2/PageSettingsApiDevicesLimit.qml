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

                headerText: qsTr("Your Amnezia Premium subscription already has the maximum number of devices — ") + ApiAccountInfoModel.data("connectedDevices")
                descriptionText: qsTr("Remove one of the previously connected devices to add a new one")
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                text: qsTr("Show all devices")

                clickedFunc: function() {
                    PageController.goToPage(PageEnum.PageSettingsApiDevices)
                }
            }
        }
    }
}


