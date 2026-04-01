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
    property int homePingMs: -1
    readonly property string homePingDisplay: homePingMs >= 0 ? (homePingMs + " ms") : "—"
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
        return FBLinkController.vipAdBlockStatus === "applied" ? "success" : "warning"
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

    Connections {
        target: FBLinkController
        function onNewFeaturesGuideChanged() { root.refreshNewFeaturesGuide() }
        function onSubscriptionChanged() { root.refreshNewFeaturesGuide() }
    }

    Component.onCompleted: root.refreshNewFeaturesGuide()
    onVisibleChanged: {
        if (visible) {
            root.refreshNewFeaturesGuide()
        }
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

                visible: false

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

                visible: false

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
            id: safeModeBanner
            visible: FBLinkController.safeModeActive
            width: root.contentWidth
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: topBar.bottom
            anchors.topMargin: 12
            padding: 12
            accentVisible: true
            accentColor: "#F59E0B"

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                PremiumBadge {
                    text: qsTr("Безопасный режим")
                    tone: "warning"
                    iconSource: "qrc:/images/controls/alert-circle.svg"
                }
                PremiumBadge {
                    text: qsTr("Автоподключение выключено")
                    tone: "neutral"
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                text: FBLinkController.safeModeUntilText !== ""
                    ? qsTr("Приложение временно запущено в безопасном режиме до %1.").arg(FBLinkController.safeModeUntilText)
                    : qsTr("Приложение временно запущено в безопасном режиме.")
                font.pixelSize: 13
                color: FBLinkStyle.color.mutedGray
                wrapMode: Text.WordWrap
            }

            BasicButtonType {
                Layout.fillWidth: true
                implicitHeight: 40
                text: qsTr("Вернуться в обычный режим")
                defaultColor: Qt.rgba(245/255, 158/255, 11/255, 0.18)
                hoveredColor: Qt.rgba(245/255, 158/255, 11/255, 0.28)
                pressedColor: Qt.rgba(245/255, 158/255, 11/255, 0.36)
                textColor: "#FFFFFF"
                clickedFunc: function() { FBLinkController.exitSafeMode() }
            }
        }

        PremiumPanel {
            id: statusCard
            width: root.contentWidth
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: FBLinkController.safeModeActive ? safeModeBanner.bottom : topBar.bottom
            anchors.topMargin: 14
            padding: 12
            accentVisible: true
            accentColor: ConnectionController.isConnected ? "#10B981" : "#00C8FF"

            Flow {
                Layout.fillWidth: true
                width: statusCard.width - statusCard.padding * 2
                spacing: 8

                PremiumBadge {
                    id: subscriptionBadge
                    compact: root.compactHeaderBadges
                    text: FBLinkController.isSubscribed
                        ? (FBLinkController.subscriptionPlan === "vip" ? qsTr("VIP") : qsTr("Premium"))
                        : qsTr("Free")
                    tone: FBLinkController.isSubscribed ? "success" : "accent"
                    iconSource: "qrc:/images/controls/shield-tick.svg"
                    interactive: true
                    hovered: subscriptionBadgeMouse.containsMouse
                    pressed: subscriptionBadgeMouse.pressed

                    MouseArea {
                        id: subscriptionBadgeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openSubscriptionPage()
                    }
                }

                PremiumBadge {
                    id: connectionBadge
                    compact: root.compactHeaderBadges
                    text: ConnectionController.isConnected ? qsTr("Подключено") : qsTr("Ожидание")
                    tone: ConnectionController.isConnected ? "success" : "accent"
                    iconSource: ConnectionController.isConnected
                        ? "qrc:/images/controls/check.svg"
                        : "qrc:/images/controls/radio.svg"
                    interactive: true
                    hovered: connectionBadgeMouse.containsMouse
                    pressed: connectionBadgeMouse.pressed

                    MouseArea {
                        id: connectionBadgeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: PageController.goToPage(PageEnum.PageSettingsConnection)
                    }
                }

                PremiumBadge {
                    id: routingBadge
                    visible: FBLinkController.canManageRoutingProfiles
                    compact: root.compactHeaderBadges
                    text: root.compactHeaderBadges ? qsTr("Routing") : qsTr("VIP routing")
                    tone: "proxy"
                    iconSource: "qrc:/images/controls/tag.svg"
                    interactive: true
                    hovered: routingBadgeMouse.containsMouse
                    pressed: routingBadgeMouse.pressed

                    MouseArea {
                        id: routingBadgeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openVipRoutingPage()
                    }
                }

                PremiumBadge {
                    id: adBlockBadge
                    compact: root.compactHeaderBadges
                    text: root.adBlockBadgeText
                    tone: root.adBlockBadgeTone
                    iconSource: "qrc:/images/controls/shield-tick.svg"
                    interactive: true
                    hovered: adBlockBadgeMouse.containsMouse
                    pressed: adBlockBadgeMouse.pressed

                    MouseArea {
                        id: adBlockBadgeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openVipRoutingPage()
                    }
                }

                PremiumBadge {
                    visible: root.homePingMs >= 0
                    compact: root.compactHeaderBadges
                    text: root.homePingDisplay
                    tone: root.homePingMs < 80 ? "success" : (root.homePingMs < 150 ? "warning" : "neutral")
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
            visible: !newFeaturesPopup.visible
            enabled: visible
            anchors.horizontalCenter: parent.horizontalCenter
            y: {
                var minY = statusCard.y + statusCard.height + 20
                var maxY = parent.height - drawer.collapsedHeight
                           - (adLabel.visible ? adLabel.implicitHeight + 24 : 24)
                           - height
                if (maxY <= minY) {
                    return minY
                }
                return minY + (maxY - minY) / 2
            }
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
            border.color: Qt.rgba(0, 200/255, 255/255, 0.35)
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
                color: Qt.rgba(0, 200/255, 255/255, 0.8)
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
                text: qsTr("Настройте маршруты для нужных сервисов и включите AdBlock в пару касаний.")
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
                    visible: FBLinkController.canUseAdBlock
                    text: qsTr("AdBlock для VIP")
                    tone: "success"
                    compact: true
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                implicitHeight: 46
                text: qsTr("Настроить VIP маршруты")
                defaultColor: "#00C8FF"
                hoveredColor: "#33D4FF"
                pressedColor: "#0099BB"
                textColor: "#FFFFFF"
                clickedFunc: function() {
                    FBLinkController.dismissNewFeaturesGuide()
                    newFeaturesPopup.close()
                    root.openVipRoutingPage()
                }
            }

            BasicButtonType {
                Layout.fillWidth: true
                implicitHeight: 46
                visible: FBLinkController.canUseAdBlock
                text: FBLinkController.vipAdBlockEnabled ? qsTr("Управлять AdBlock") : qsTr("Включить AdBlock")
                defaultColor: Qt.rgba(16/255, 185/255, 129/255, 0.20)
                hoveredColor: Qt.rgba(16/255, 185/255, 129/255, 0.30)
                pressedColor: Qt.rgba(16/255, 185/255, 129/255, 0.38)
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
                            root.homePingMs = ms
                        }
                    }

                    function measurePing() {
                        var pingTarget = ServersModel.getDefaultServerPingTarget() || ""
                        if (pingTarget === "") {
                            locationCardRef.realPingMs = -1
                            root.homePingMs = -1
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
