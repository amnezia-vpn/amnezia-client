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
    readonly property bool compactHeaderBadges: width < 420
    readonly property real contentWidth: Math.min(width - 32, GC.pageMaxWidth(width))
    readonly property real contentSideMargin: Math.max(16, (width - contentWidth) / 2)
    property bool connectionCardsVisibleState: false
    property bool connectionCardsRender: false
    property int homePingMs: -1
    readonly property string homePingDisplay: homePingMs >= 0 ? (homePingMs + " ms") : "—"
    readonly property string homeEndpointDisplay: ServersModel.defaultServerEndpointHost !== ""
        ? ServersModel.defaultServerEndpointHost
        : "—"
    readonly property string homeStateTitle: ConnectionController.isConnected
        ? qsTr("VPN активен")
        : (ConnectionController.isConnectionInProgress ? qsTr("Подготавливаем подключение") : qsTr("Готово к подключению"))
    readonly property string homeStateSubtitle: FBLinkController.isLoading
        ? qsTr("Обновление конфигураций...")
        : (ServersModel.defaultServerName !== ""
            ? ServersModel.defaultServerName
            : qsTr("Выберите локацию и нажмите подключение"))
    readonly property string adBlockBadgeText: {
        if (!FBLinkController.canUseAdBlock) {
            return qsTr("AdBlock недоступен")
        }
        return qsTr("AdBlock: %1").arg(FBLinkController.vipAdBlockStatusLabel)
    }
    readonly property string adBlockBadgeTone: {
        if (!FBLinkController.canUseAdBlock) {
            return "neutral"
        }
        if (!FBLinkController.vipAdBlockEnabled) {
            return "neutral"
        }
        return (FBLinkController.vipAdBlockStatus === "degraded"
            || FBLinkController.vipAdBlockStatus === "unavailable") ? "warning" : "success"
    }

    function openSubscriptionPage() {
        if (!FBLinkController.isLoggedIn) {
            PageController.goToPage(PageEnum.PageFBLinkLogin)
            return
        }
        PageController.goToPage(PageEnum.PageFBLinkSubscription)
    }

    function openVipRoutingPage() {
        if (!FBLinkController.isLoggedIn) {
            PageController.goToPage(PageEnum.PageFBLinkLogin)
            return
        }
        PageController.goToPage(PageEnum.PageSettingsVipRoutingProfiles)
    }

    function refreshNewFeaturesGuide() {
        if (!root.visible || !FBLinkController.isSubscribed || !FBLinkController.showNewFeaturesGuide) {
            return
        }
        if (!newFeaturesPopup.visible) {
            newFeaturesPopup.open()
        }
    }

    function refreshPing() {
        const pingTarget = ServersModel.getDefaultServerPingTarget() || ""
        if (pingTarget === "") {
            root.homePingMs = -1
            return
        }
        SystemController.measurePing(pingTarget)
    }

    function updateConnectionCards(connected) {
        if (connected) {
            connectionCardsHideTimer.stop()
            connectionCardsRender = true
            connectionCardsVisibleState = true
        } else {
            connectionCardsVisibleState = false
            if (connectionCardsRender) {
                connectionCardsHideTimer.restart()
            }
        }
    }

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
            // Server list is now opened from the dedicated middle tab button.
        }
    }

    Connections {
        target: SystemController
        function onPingMeasured(ms) {
            root.homePingMs = ms
        }
    }

    Connections {
        target: ServersModel
        function onDefaultServerIndexChanged(index) {
            root.homePingMs = -1
            if (ConnectionController.isConnected) {
                root.refreshPing()
            }
        }
        function onDefaultServerDefaultContainerChanged(containerIndex) {
            root.homePingMs = -1
            if (ConnectionController.isConnected) {
                root.refreshPing()
            }
        }
    }

    Connections {
        target: ConnectionController
        function onConnectionStateChanged() {
            if (ConnectionController.isConnected) {
                root.refreshPing()
            } else {
                root.homePingMs = -1
            }
            root.updateConnectionCards(ConnectionController.isConnected)
        }
    }

    Connections {
        target: FBLinkController
        function onNewFeaturesGuideChanged() { root.refreshNewFeaturesGuide() }
        function onSubscriptionChanged() { root.refreshNewFeaturesGuide() }
    }

    Component.onCompleted: {
        root.refreshNewFeaturesGuide()
        root.updateConnectionCards(ConnectionController.isConnected)
    }
    onVisibleChanged: {
        if (visible) {
            root.refreshNewFeaturesGuide()
        }
    }

    Timer {
        id: homePingTimer
        interval: 5000
        running: ConnectionController.isConnected
        repeat: true
        triggeredOnStart: true
        onTriggered: root.refreshPing()
    }

    Timer {
        id: connectionCardsHideTimer
        interval: 260
        repeat: false
        onTriggered: root.connectionCardsRender = false
    }


    Flickable {
        id: homeFlickable
        objectName: "homeColumnItem"

        anchors.fill: parent
        anchors.bottomMargin: drawer.collapsedHeight + subscriptionBanner.implicitHeight
        contentWidth: width
        contentHeight: height
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragAndOvershootBounds

        Item {
            id: ptrIndicator
            width: homeFlickable.width
            height: 60
            y: -80
            visible: FBLinkController.isLoading || homeFlickable.contentY < 0

            RowLayout {
                anchors.centerIn: parent
                spacing: 12
                
                BusyIndicator {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    running: FBLinkController.isLoading || homeFlickable.contentY < -40
                    visible: running
                }
                
                LabelTextType {
                    text: FBLinkController.isLoading 
                        ? qsTr("Обновление конфигураций...") 
                        : (homeFlickable.contentY < -70 ? qsTr("Отпустите для обновления") : qsTr("Потяните для обновления"))
                    font.pixelSize: 13
                    color: FBLinkStyle.color.mutedGray
                }
            }
        }

        onMovementEnded: {
            if (contentY < -70 && !FBLinkController.isLoading) {
                if (FBLinkController.isLoggedIn) {
                    FBLinkController.syncAll()
                } else {
                    FBLinkController.fetchConfig() // triggers error message
                }
            }
        }

        PremiumPanel {
            id: safeModeBanner
            visible: FBLinkController.safeModeActive
            width: root.contentWidth
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 14 + SettingsController.safeAreaTopMargin
            padding: 14
            fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
            outlineColor: Qt.rgba(245/255, 158/255, 11/255, 0.38)
            accentVisible: true
            accentColor: "#F59E0B"
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    radius: 11
                    color: Qt.rgba(245/255, 158/255, 11/255, 0.14)
                    border.width: 1
                    border.color: Qt.rgba(245/255, 158/255, 11/255, 0.45)

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/images/controls/alert-circle.svg"
                        sourceSize: Qt.size(18, 18)
                        layer.enabled: true
                        layer.effect: ColorOverlay { color: "#F59E0B" }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("Безопасный запуск")
                        font.pixelSize: 15
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        elide: Text.ElideRight
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: FBLinkController.safeModeUntilText !== ""
                            ? qsTr("Автоподключение временно выключено до %1").arg(FBLinkController.safeModeUntilText)
                            : qsTr("Автоподключение временно выключено")
                        color: FBLinkStyle.color.mutedGray
                        elide: Text.ElideRight
                    }
                }

                PremiumBadge {
                    text: qsTr("SAFE")
                    tone: "warning"
                    compact: true
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                implicitHeight: 42
                text: qsTr("Вернуться в обычный режим")
                defaultColor: Qt.rgba(245/255, 158/255, 11/255, 0.18)
                hoveredColor: Qt.rgba(245/255, 158/255, 11/255, 0.28)
                pressedColor: Qt.rgba(245/255, 158/255, 11/255, 0.36)
                textColor: "#FFFFFF"
                clickedFunc: function() { FBLinkController.exitSafeMode() }
            }
        }

        Item {
            id: homeCenterStage
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: FBLinkController.safeModeActive ? safeModeBanner.bottom : parent.top
            anchors.bottom: adLabel.top
            anchors.topMargin: FBLinkController.safeModeActive ? 14 : (16 + SettingsController.safeAreaTopMargin)
            anchors.bottomMargin: 20

            ColumnLayout {
                width: Math.min(root.contentWidth, 520)
                anchors.centerIn: parent
                spacing: 16

                ConnectButton {
                    id: connectButton
                    objectName: "connectButton"
                    Layout.alignment: Qt.AlignHCenter
                    visible: !newFeaturesPopup.visible
                    enabled: visible
                }

                Rectangle {
                    visible: true
                    Layout.fillWidth: true
                    implicitHeight: 86
                    radius: 16
                    color: locationCardMouse.containsMouse
                        ? Qt.rgba(24/255, 24/255, 24/255, 1.0)
                        : Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 14

                        Rectangle {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 46
                            Layout.alignment: Qt.AlignVCenter
                            radius: 12
                            color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                            border.width: 1
                            border.color: Qt.rgba(63/255, 63/255, 70/255, 0.7)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/map-pin.svg"
                                sourceSize: Qt.size(22, 22)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#EAB308" }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Выбранная локация")
                                font.pixelSize: 11
                                color: FBLinkStyle.color.mutedGray
                                font.capitalization: Font.AllUppercase
                                elide: Text.ElideRight
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: ServersModel.defaultServerName !== ""
                                    ? ServersModel.defaultServerName
                                    : qsTr("Выберите локацию")
                                font.pixelSize: 18
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                                elide: Text.ElideRight
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                visible: ConnectionController.isConnected
                                text: root.homePingMs >= 0
                                    ? qsTr("%1 · %2 ms").arg(ServersModel.defaultServerDefaultContainerName).arg(root.homePingMs)
                                    : ServersModel.defaultServerDefaultContainerName
                                color: root.homePingMs < 0 ? FBLinkStyle.color.mutedGray
                                     : (root.homePingMs < 80 ? "#10B981"
                                     : (root.homePingMs < 150 ? "#F59E0B" : "#EF4444"))
                                elide: Text.ElideRight
                            }
                        }

                        Item {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignVCenter

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/chevron-right.svg"
                                sourceSize: Qt.size(20, 20)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: FBLinkStyle.color.charcoalGray }
                            }
                        }
                    }

                    Rectangle {
                        visible: FBLinkController.isSubscribed && FBLinkController.subscriptionPlan === "vip"
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 8
                        anchors.rightMargin: 8
                        radius: 6
                        color: "#EAB308"
                        implicitWidth: vipTagText.implicitWidth + 8
                        implicitHeight: vipTagText.implicitHeight + 4

                        CaptionTextType {
                            id: vipTagText
                            anchors.centerIn: parent
                            text: "VIP"
                            color: "#111111"
                            font.bold: true
                            font.pixelSize: 9
                        }
                    }

                    MouseArea {
                        id: locationCardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: PageController.goToPage(PageEnum.PageSettingsServersList)
                    }
                }

                GridLayout {
                    id: connectionInfoCards
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 12
                    visible: root.connectionCardsRender

                    Rectangle {
                        id: ipInfoCard
                        Layout.fillWidth: true
                        implicitHeight: 84
                        radius: 14
                        color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                        border.width: 1
                        border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                        property real revealOffset: root.connectionCardsVisibleState ? 0 : 14
                        opacity: root.connectionCardsVisibleState ? 1 : 0
                        transform: Translate { y: ipInfoCard.revealOffset }
                        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                        Behavior on revealOffset { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2

                            Image {
                                Layout.alignment: Qt.AlignHCenter
                                source: "qrc:/images/controls/map-pin.svg"
                                sourceSize: Qt.size(18, 18)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#10B981" }
                            }

                            CaptionTextType {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("IP Адрес")
                                color: FBLinkStyle.color.mutedGray
                            }

                            LabelTextType {
                                Layout.alignment: Qt.AlignHCenter
                                text: root.homeEndpointDisplay
                                color: FBLinkStyle.color.paleGray
                                font.weight: 700
                                font.pixelSize: 13
                                elide: Text.ElideMiddle
                            }
                        }
                    }

                    Rectangle {
                        id: protocolInfoCard
                        Layout.fillWidth: true
                        implicitHeight: 84
                        radius: 14
                        color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                        border.width: 1
                        border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                        property real revealOffset: root.connectionCardsVisibleState ? 0 : 14
                        opacity: root.connectionCardsVisibleState ? 1 : 0
                        transform: Translate { y: protocolInfoCard.revealOffset }
                        Behavior on opacity {
                            SequentialAnimation {
                                PauseAnimation { duration: 60 }
                                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                            }
                        }
                        Behavior on revealOffset {
                            SequentialAnimation {
                                PauseAnimation { duration: 60 }
                                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2

                            Image {
                                Layout.alignment: Qt.AlignHCenter
                                source: "qrc:/images/controls/radio.svg"
                                sourceSize: Qt.size(18, 18)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#10B981" }
                            }

                            CaptionTextType {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("Протокол")
                                color: FBLinkStyle.color.mutedGray
                            }

                            LabelTextType {
                                Layout.alignment: Qt.AlignHCenter
                                text: ServersModel.defaultServerDefaultContainerName !== ""
                                    ? ServersModel.defaultServerDefaultContainerName
                                    : qsTr("Неизвестно")
                                color: FBLinkStyle.color.paleGray
                                font.weight: 700
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }

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

    Popup {
        id: newFeaturesPopup
        width: Math.min(root.width - 28, 560)
        x: (root.width - width) / 2
        y: Math.max(24 + SettingsController.safeAreaTopMargin, (root.height - implicitHeight) / 2)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 20

        onClosed: {
            if (FBLinkController.showNewFeaturesGuide) {
                FBLinkController.dismissNewFeaturesGuide()
            }
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(0, 0, 0, 0.56)
        }

        background: Rectangle {
            clip: true
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(27/255, 30/255, 41/255, 0.99) }
                GradientStop { position: 1.0; color: Qt.rgba(19/255, 22/255, 31/255, 0.99) }
            }
            radius: 20
            border.color: Qt.rgba(234/255, 179/255, 8/255, 0.35)
            border.width: 1

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.topMargin: 10
                height: 3
                radius: 2
                color: Qt.rgba(234/255, 179/255, 8/255, 0.8)
            }

        }

        contentItem: ColumnLayout {
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                PremiumBadge {
                    text: qsTr("VIP")
                    tone: "proxy"
                    iconSource: "qrc:/images/controls/shield-tick.svg"
                }

                PremiumBadge {
                    text: qsTr("Новое")
                    tone: "accent"
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 30
                    height: 30
                    radius: 15
                    color: closeGuideMouse.pressed
                        ? Qt.rgba(1, 1, 1, 0.20)
                        : (closeGuideMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.10))
                    border.color: Qt.rgba(1, 1, 1, 0.12)
                    border.width: 1

                    Text {
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: "×"
                        color: FBLinkStyle.color.paleGray
                        font.pixelSize: 18
                        font.family: "PT Root UI VF"
                        font.weight: 500
                    }

                    MouseArea {
                        id: closeGuideMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            FBLinkController.dismissNewFeaturesGuide()
                            newFeaturesPopup.close()
                        }
                    }
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                text: qsTr("Новые возможности готовы")
                font.pixelSize: 24
                font.weight: 700
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
            }

            CaptionTextType {
                Layout.fillWidth: true
                text: qsTr("Настройте маршруты для нужных сервисов в пару касаний.")
                color: FBLinkStyle.color.mutedGray
                wrapMode: Text.WordWrap
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                PremiumBadge {
                    text: qsTr("Маршруты сервисов")
                    tone: "proxy"
                    compact: true
                }

                PremiumBadge {
                    visible: false
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                implicitHeight: 46
                text: qsTr("Настроить VIP маршруты")
                defaultColor: "#EAB308"
                hoveredColor: "#FACC15"
                pressedColor: "#CA8A04"
                textColor: "#FFFFFF"
                clickedFunc: function() {
                    FBLinkController.dismissNewFeaturesGuide()
                    newFeaturesPopup.close()
                    root.openVipRoutingPage()
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                implicitHeight: 42
                text: qsTr("Позже")
                defaultColor: Qt.rgba(1, 1, 1, 0.08)
                hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                pressedColor: Qt.rgba(1, 1, 1, 0.18)
                textColor: FBLinkStyle.color.paleGray
                clickedFunc: function() {
                    FBLinkController.dismissNewFeaturesGuide()
                    newFeaturesPopup.close()
                }
            }
        }
    }

    DrawerType2 {
        id: drawer
        objectName: "drawerProtocol"

        anchors.fill: parent
        visible: false
        enabled: false
        collapsedHeight: 0
        expandedHeight: 0

        collapsedStateContent: Item {
            objectName: "ProtocolDrawerCollapsedContent"

            implicitHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : root.height * 0.77
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
                    drawer.collapsedHeight = 0
                }

                Rectangle {
                    id: bottomActionPanel
                    Layout.fillWidth: true
                    Layout.leftMargin: root.contentSideMargin
                    Layout.rightMargin: root.contentSideMargin
                    Layout.topMargin: drawer.isCollapsedStateActive ? 14 : 8
                    Layout.bottomMargin: drawer.isCollapsedStateActive ? (16 + SettingsController.safeAreaBottomMargin) : 12
                    Layout.preferredHeight: 70

                    color: "#171717"
                    radius: 16
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Панель быстрого доступа")
                                font.pixelSize: 12
                                font.weight: 500
                                color: FBLinkStyle.color.mutedGray
                                elide: Text.ElideRight
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: ServersModel.defaultServerName !== "" ? ServersModel.defaultServerName : qsTr("Локация не выбрана")
                                font.pixelSize: 15
                                font.weight: 700
                                color: "#FFFFFF"
                                elide: Text.ElideRight
                            }
                        }

                        BasicButtonType {
                            Layout.preferredWidth: 132
                            implicitHeight: 42
                            text: qsTr("Локации")
                            defaultColor: Qt.rgba(234/255, 179/255, 8/255, 0.16)
                            hoveredColor: Qt.rgba(234/255, 179/255, 8/255, 0.24)
                            pressedColor: Qt.rgba(234/255, 179/255, 8/255, 0.30)
                            textColor: "#FFFFFF"
                            leftImageSource: "qrc:/images/controls/map-pin.svg"
                            clickedFunc: function() {
                                drawer.openTriggered()
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
