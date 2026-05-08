import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    readonly property int activeDevices: ApiAccountInfoModel.data("activeDeviceCount")
    readonly property int maxDevices: ApiAccountInfoModel.data("maxDeviceCount")

    FlickableType {
        anchors.fill: parent
        contentHeight: layout.implicitHeight

        ColumnLayout {
            id: layout
            width: root.width
            spacing: 16

            BackButtonType {
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                text: qsTr("Device limit reached")
                font.pixelSize: 28
                font.bold: true
                color: AmneziaStyle.color.paleGray
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: root.maxDevices > 0
                      ? qsTr("The maximum number of devices is already in use (%1 of %2). Remove a device to add a new one.")
                        .arg(root.activeDevices).arg(root.maxDevices)
                      : qsTr("The maximum number of devices for this subscription is already in use. Remove a device to add a new one.")
                wrapMode: Text.Wrap
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                Layout.bottomMargin: 24 + PageController.safeAreaBottomMargin

                text: qsTr("View All Devices")
                defaultColor: AmneziaStyle.color.paleGray
                hoveredColor: AmneziaStyle.color.lightGray
                pressedColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.midnightBlack

                clickedFunc: function() {
                    PageController.closePage()
                }
            }
        }
    }
}
