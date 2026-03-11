import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ContainersModelFilters 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

ListViewType {
    id: root

    property int selectedIndex: ServersModel.defaultIndex

    anchors.top: serversMenuHeader.bottom
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.topMargin: 16

    model: ServersModel

    Connections {
        target: ServersModel
        function onDefaultServerIndexChanged(serverIndex) {
            root.selectedIndex = serverIndex
        }
    }

    delegate: Item {
        id: menuContentDelegate
        objectName: "menuContentDelegate"

        property variant delegateData: model

        implicitWidth: root.width
        implicitHeight: 76 // 64 for card + 12 for spacing

        Rectangle {
            id: serverCard
            
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 16
            anchors.leftMargin: 16
            height: 64
            
            radius: 12
            color: index === root.selectedIndex ? "#1F1F24" : (mouseArea.pressed ? "#2A2A30" : "transparent")
            border.color: index === root.selectedIndex ? "#00C8FF" : "transparent"
            border.width: index === root.selectedIndex ? 2 : 0

            Behavior on color { ColorAnimation { duration: 150 } }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                // Location Icon
                Image {
                    Layout.alignment: Qt.AlignVCenter
                    source: "qrc:/images/controls/map-pin.svg"
                    sourceSize: Qt.size(24, 24)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    LabelTextType {
                        Layout.fillWidth: true
                        text: name
                        font.pixelSize: 16
                        font.weight: index === root.selectedIndex ? 600 : 400
                        color: index === root.selectedIndex ? "#FFFFFF" : "#EBEBF5"
                        elide: Text.ElideRight
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: serverDescription
                        font.pixelSize: 13
                        color: index === root.selectedIndex ? "#00C8FF" : "#8A8A8E"
                        visible: serverDescription !== "" && serverDescription !== name
                        elide: Text.ElideRight
                    }
                }

                // Checkmark for selected item
                Image {
                    Layout.alignment: Qt.AlignVCenter
                    source: "qrc:/images/controls/check.svg" // Assumes standard check icon exists
                    sourceSize: Qt.size(20, 20)
                    visible: index === root.selectedIndex
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Нельзя менять сервер во время активного подключения"))
                        return
                    }
                    root.selectedIndex = index
                    ServersModel.defaultIndex = index
                    drawer.closeTriggered()
                }
            }
        }
    }
}
