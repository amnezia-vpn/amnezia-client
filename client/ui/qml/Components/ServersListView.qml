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
        property bool lockedByPlan: isVipOnly && !isAvailableForCurrentPlan

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
            opacity: lockedByPlan ? 0.62 : 1.0
            
            radius: 12
            color: index === root.selectedIndex ? "#1F1F24" : (mouseArea.pressed ? "#2A2A30" : "transparent")
            border.color: index === root.selectedIndex ? "#EAB308" : "transparent"
            border.width: index === root.selectedIndex ? 2 : 0

            Behavior on color { ColorAnimation { duration: 150 } }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                // Flag or location icon
                Image {
                    Layout.alignment: Qt.AlignVCenter
                    source: apiServerCountryCode !== ""
                            ? "qrc:/countriesFlags/images/flagKit/" + apiServerCountryCode + ".svg"
                            : "qrc:/images/controls/map-pin.svg"
                    sourceSize: apiServerCountryCode !== "" ? Qt.size(32, 22) : Qt.size(24, 24)
                    fillMode: Image.PreserveAspectFit
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

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        PremiumBadge {
                            visible: isVipOnly
                            text: lockedByPlan ? qsTr("VIP") : qsTr("VIP server")
                            tone: "accent"
                            iconSource: "qrc:/images/controls/crown.svg"
                            compact: true
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: lockedByPlan
                                ? qsTr("Р”РѕСЃС‚СѓРїРµРЅ С‚РѕР»СЊРєРѕ РґР»СЏ VIP")
                                : serverDescription
                            font.pixelSize: 13
                            color: index === root.selectedIndex ? "#EAB308" : "#8A8A8E"
                            visible: text !== "" && text !== name
                            elide: Text.ElideRight
                        }
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
                    if (lockedByPlan) {
                        PageController.showNotificationMessage(qsTr("Р­С‚РѕС‚ СЃРµСЂРІРµСЂ РґРѕСЃС‚СѓРїРµРЅ С‚РѕР»СЊРєРѕ РґР»СЏ VIP"))
                        return
                    }
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
