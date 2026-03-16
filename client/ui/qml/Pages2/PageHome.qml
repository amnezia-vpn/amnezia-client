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
        anchors.bottomMargin: drawer.collapsedHeight + subscriptionBanner.implicitHeight

        // ── Top bar: Premium button ──────────────────────────────
        RowLayout {
            id: topBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 14 + SettingsController.safeAreaTopMargin
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Item { Layout.fillWidth: true }

            // Premium badge button
            Rectangle {
                id: premiumBtn
                width: premiumBtnRow.implicitWidth + 20
                height: 34
                radius: 17

                visible: !FBLinkController.isSubscribed

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#33D4FF" }
                    GradientStop { position: 1.0; color: "#00C8FF" }
                }

                border.color: Qt.rgba(1, 1, 1, 0.15)
                border.width: 1

                scale: premiumBtnMouse.pressed ? 0.93 : (premiumBtnMouse.containsMouse ? 1.04 : 1.0)
                Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }

                RowLayout {
                    id: premiumBtnRow
                    anchors.centerIn: parent
                    spacing: 5

                    Image {
                        source: "qrc:/images/controls/shield-tick.svg"
                        sourceSize: Qt.size(14, 14)
                        layer.enabled: true
                        layer.effect: ColorOverlay { color: "#FFFFFF" }
                    }

                    LabelTextType {
                        text: FBLinkController.isLoggedIn
                            ? qsTr("Получить Premium")
                            : qsTr("Войти")
                        font.pixelSize: 13
                        font.weight: 700
                        color: "#FFFFFF"
                    }
                }

                MouseArea {
                    id: premiumBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (FBLinkController.isLoggedIn) {
                            PageController.goToPage(PageEnum.PageFBLinkSubscription)
                        } else {
                            PageController.goToPage(PageEnum.PageFBLinkLogin)
                        }
                    }
                }
            }

            // Active subscription badge
            Rectangle {
                width: activeBadgeRow.implicitWidth + 20
                height: 34
                radius: 17

                visible: FBLinkController.isSubscribed

                color: Qt.rgba(16/255, 185/255, 129/255, 0.18)
                border.color: Qt.rgba(16/255, 185/255, 129/255, 0.4)
                border.width: 1

                RowLayout {
                    id: activeBadgeRow
                    anchors.centerIn: parent
                    spacing: 5

                    Image {
                        source: "qrc:/images/controls/shield-tick.svg"
                        sourceSize: Qt.size(14, 14)
                        layer.enabled: true
                        layer.effect: ColorOverlay { color: "#10B981" }
                    }

                    LabelTextType {
                        text: qsTr("Premium")
                        font.pixelSize: 13
                        font.weight: 700
                        color: "#10B981"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: PageController.goToPage(PageEnum.PageFBLinkSubscription)
                }
            }
        }

        // ── Connect button centered ──────────────────────────────
        ConnectButton {
            id: connectButton
            objectName: "connectButton"

            z: 99
            anchors.centerIn: parent
            // Shift down by half topBar's height so button is centered
            // in the visual space below the top bar, not the whole Item
            anchors.verticalCenterOffset: (topBar.y + topBar.height) / 2
                                          - (adLabel.visible ? adLabel.implicitHeight / 2 + 11 : 0)
        }

        // ── Ad label below button ────────────────────────────────
        AdLabel {
            id: adLabel

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 16
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            height: contentHeight
        }
    }

    // ── Subscription Banner ─────────────────────────────────────────
    SubscriptionBanner {
        id: subscriptionBanner

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: drawer.collapsedHeight
        }

        // Hide banner when drawer is expanded (server list is open)
        opacity: drawer.isCollapsedStateActive() ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 200 } }
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
                    id: locationCardRef
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

                    property int fakePing: 42
                    Timer {
                        interval: 2500
                        running: ConnectionController.isConnected
                        repeat: true
                        onTriggered: locationCardRef.fakePing = 35 + Math.floor(Math.random() * 15)
                    }

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

                            scale: ConnectionController.isConnected ? 1.15 : 1.0
                            Behavior on scale {
                                NumberAnimation {
                                    duration: 400
                                    easing.type: Easing.OutBack
                                }
                            }
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

                        // Fake Ping / Chevron
                        RowLayout {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 8

                            LabelTextType {
                                text: "• " + locationCardRef.fakePing + " ms"
                                color: "#10B981"
                                font.pixelSize: 13
                                font.weight: 600
                                visible: ConnectionController.isConnected
                            }

                            // Chevron
                            Image {
                                source: "qrc:/images/controls/chevron-up.svg"
                                sourceSize: Qt.size(20, 20)
                                rotation: drawer.isCollapsedStateActive ? 0 : 180
                                Behavior on rotation { NumberAnimation { duration: 200 } }
                            }
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

                anchors.top: parent.top
                anchors.topMargin: drawer.collapsedHeight
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
