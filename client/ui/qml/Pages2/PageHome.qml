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
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real contentWidth: Math.min(width - 32, GC.pageMaxWidth(width))
    readonly property real contentSideMargin: Math.max(16, (width - contentWidth) / 2)
    readonly property string homeStateTitle: ConnectionController.isConnected
        ? qsTr("VPN активен")
        : (ConnectionController.isConnectionInProgress ? qsTr("Подготавливаем подключение") : qsTr("Готово к подключению"))
    readonly property string homeStateSubtitle: ServersModel.defaultServerName !== ""
        ? ServersModel.defaultServerName
        : qsTr("Выберите локацию и нажмите подключение")

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
            anchors.leftMargin: root.contentSideMargin
            anchors.rightMargin: root.contentSideMargin

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
                            ? qsTr("Получить Премиум")
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
                        text: FBLinkController.subscriptionPlan === "vip"
                            ? qsTr("VIP")
                            : qsTr("Премиум")
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

        PremiumPanel {
            id: statusCard
            width: root.contentWidth
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: connectButton.top
            anchors.bottomMargin: 18
            padding: 12
            accentVisible: true
            accentColor: ConnectionController.isConnected ? "#10B981" : "#00C8FF"

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                PremiumBadge {
                    text: FBLinkController.isSubscribed
                        ? (FBLinkController.subscriptionPlan === "vip" ? qsTr("VIP") : qsTr("Premium"))
                        : qsTr("Free")
                    tone: FBLinkController.isSubscribed ? "success" : "accent"
                    iconSource: "qrc:/images/controls/shield-tick.svg"
                }

                PremiumBadge {
                    text: ConnectionController.isConnected ? qsTr("Подключено") : qsTr("Ожидание")
                    tone: ConnectionController.isConnected ? "success" : "accent"
                    iconSource: ConnectionController.isConnected
                        ? "qrc:/images/controls/check.svg"
                        : "qrc:/images/controls/radio.svg"
                }

                PremiumBadge {
                    visible: FBLinkController.canManageRoutingProfiles
                    text: qsTr("VIP routing")
                    tone: "proxy"
                    iconSource: "qrc:/images/controls/tag.svg"
                }

                PremiumBadge {
                    visible: locationCardRef.realPingMs >= 0
                    text: locationCardRef.pingDisplay
                    tone: locationCardRef.realPingMs < 80 ? "success" : (locationCardRef.realPingMs < 150 ? "warning" : "neutral")
                    iconSource: "qrc:/images/controls/gauge.svg"
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                text: root.homeStateTitle
                font.pixelSize: 17
                font.weight: 700
                color: FBLinkStyle.color.paleGray
                wrapMode: Text.WordWrap
            }

            LabelTextType {
                Layout.fillWidth: true
                text: root.homeStateSubtitle
                font.pixelSize: 13
                color: FBLinkStyle.color.mutedGray
                wrapMode: Text.WordWrap
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
            anchors.leftMargin: root.contentSideMargin
            anchors.rightMargin: root.contentSideMargin

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
                    Layout.leftMargin: root.contentSideMargin
                    Layout.rightMargin: root.contentSideMargin
                    Layout.topMargin: drawer.isCollapsedStateActive ? 16 : 8
                    Layout.bottomMargin: drawer.isCollapsedStateActive ? 48 + SettingsController.safeAreaBottomMargin : 16
                    height: 64

                    color: locationMouseArea.pressed ? "#1F1F24" : (locationMouseArea.containsMouse ? "#2A2A30" : "#24242A")
                    radius: 16
                    border.color: "#333333"
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }

                    // Кроссплатформенный TCP-пинг через C++ SystemController (работает на iOS, Android, Desktop)
                    property int realPingMs: -1
                    property string pingDisplay: realPingMs >= 0 ? (realPingMs + " ms") : "—"

                    Connections {
                        target: SystemController
                        function onPingMeasured(ms) {
                            locationCardRef.realPingMs = ms
                        }
                    }

                    function measurePing() {
                        var pingTarget = ServersModel.getDefaultServerPingTarget() || ""
                        if (pingTarget === "") {
                            locationCardRef.realPingMs = -1
                            return
                        }

                        SystemController.measurePing(pingTarget)
                    }

                    Timer {
                        id: pingTimer
                        interval: 5000
                        running: ConnectionController.isConnected
                        repeat: true
                        triggeredOnStart: true
                        onTriggered: locationCardRef.measurePing()
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

                        // Active endpoint ping / Chevron
                        RowLayout {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 8

                            LabelTextType {
                                text: "• " + locationCardRef.pingDisplay
                                color: locationCardRef.realPingMs < 0 ? "#8A8A8E"
                                     : locationCardRef.realPingMs < 80 ? "#10B981"
                                     : locationCardRef.realPingMs < 150 ? "#F59E0B"
                                     : "#EF4444"
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
