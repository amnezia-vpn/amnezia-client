import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

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
import "../Components"

PageType {
    id: root

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                if (drawer.isOpened) {
                    drawer.closeTriggered()
                }
                if (homeSplitTunnelingDrawer.isOpened) {
                    homeSplitTunnelingDrawer.closeTriggered()
                }
            }
        }
    }

    Connections {
        objectName: "pageControllerConnections"

        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            drawer.openTriggered()
            if (isContainerInstalled) {
                containersDropDown.rootButtonClickedFunction()
            }
        }
    }


    Item {
        objectName: "homeColumnItem"

        anchors.fill: parent
        anchors.bottomMargin: drawer.collapsedHeight

        ColumnLayout {
            objectName: "homeColumnLayout"

            anchors.fill: parent
            anchors.topMargin: 12 + SettingsController.safeAreaTopMargin
            anchors.bottomMargin: 16

            ConnectButton {
                id: connectButton
                objectName: "connectButton"

                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
            }

            AdLabel {
                id: adLabel

                Layout.fillWidth: true
                Layout.preferredHeight: adLabel.contentHeight
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 22
            }
        }
    }

    DrawerType2 {
        id: drawer
        objectName: "drawerProtocol"

        anchors.fill: parent

        collapsedStateContent: Item {
            objectName: "ProtocolDrawerCollapsedContent"

            implicitHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : screen.height * 0.77
            Component.onCompleted: {
                drawer.expandedHeight = implicitHeight
            }

            ColumnLayout {
                id: collapsed
                objectName: "collapsedColumnLayout"

                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 0

                Component.onCompleted: {
                    drawer.collapsedHeight = collapsed.implicitHeight
                }

                // Modern Location Picker Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: drawer.isCollapsedStateActive ? 16 : 8
                    Layout.bottomMargin: drawer.isCollapsedStateActive ? 48 + SettingsController.safeAreaBottomMargin : 16
                    height: 64

                    color: locationMouseArea.pressed ? "#1F1F24" : (locationMouseArea.containsMouse ? "#2A2A30" : "#24242A")
                    radius: 16
                    border.color: "#333333"
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        // Location Icon / Flag
                        Image {
                            Layout.alignment: Qt.AlignVCenter
                            source: ServersModel.defaultServerImagePathCollapsed !== "" ? ServersModel.defaultServerImagePathCollapsed : "qrc:/images/controls/map-pin.svg"
                            sourceSize: Qt.size(24, 24)
                            fillMode: Image.PreserveAspectFit
                        }

                        // Server Name & Status
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            LabelTextType {
                                Layout.fillWidth: true
                                text: ServersModel.defaultServerName !== "" ? ServersModel.defaultServerName : qsTr("Выбрать локацию")
                                font.pixelSize: 16
                                font.weight: 600
                                color: "#FFFFFF"
                                elide: Text.ElideRight
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: ConnectionController.isConnected ? qsTr("Подключено") : qsTr("Нажмите для смены региона")
                                font.pixelSize: 13
                                color: ConnectionController.isConnected ? "#10B981" : "#8A8A8E"
                                elide: Text.ElideRight
                            }
                        }

                        // Chevron
                        Image {
                            Layout.alignment: Qt.AlignVCenter
                            source: "qrc:/images/controls/chevron-up.svg"
                            sourceSize: Qt.size(20, 20)
                            rotation: drawer.isCollapsedStateActive ? 0 : 180
                            Behavior on rotation { NumberAnimation { duration: 200 } }
                        }
                    }

                    MouseArea {
                        id: locationMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (drawer.isCollapsedStateActive()) {
                                drawer.openTriggered()
                            } else {
                                drawer.closeTriggered()
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                id: serversMenuHeader
                objectName: "serversMenuHeader"

                anchors.top: collapsed.bottom
                anchors.right: parent.right
                anchors.left: parent.left

                Header2Type {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24

                    headerText: qsTr("Выбрать локацию")
                }
            }

            ServersListView {
                id: serversMenuContent
                objectName: "serversMenuContent"

                isFocusable: false

                Connections {
                    target: drawer

                    // this item shouldn't be focused when drawer is closed
                    function onIsOpenedChanged() {
                        serversMenuContent.isFocusable = drawer.isOpened
                    }
                }
            }
        }
    }
}
