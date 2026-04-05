import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    // 0 = basic, 1 = vip
    property int selectedPlan: 0
    property bool isLoading: false
    property string errorMessage: ""
    property bool isWaitingForPayment: false
    property int pollCount: 0
    property bool confirmDeleteCard: false
    property string mgmtError: ""
    readonly property int maxPolls: 60  // 60 × 2 s = 2 min
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

    readonly property int currentPlanLevel: {
        if (!FBLinkController.isSubscribed) return -1
        if (FBLinkController.subscriptionPlan === "vip") return 1
        if (FBLinkController.subscriptionPlan === "basic" || FBLinkController.subscriptionPlan === "trial") return 0
        return -1
    }
    readonly property bool showNewUserOffer: FBLinkController.trialAvailable && !FBLinkController.isSubscribed

    readonly property var selectedPlanData: root.plans[Math.max(0, Math.min(root.selectedPlan, root.plans.length - 1))]

    Timer {
        id: pollTimer
        interval: 2000
        repeat: true
        running: false
        onTriggered: {
            root.pollCount++
            if (root.pollCount >= root.maxPolls) {
                pollTimer.stop()
                root.isWaitingForPayment = false
                root.errorMessage = qsTr("Время ожидания истекло. Если вы оплатили — нажмите «Проверить вручную».")
                return
            }
            if (FBLinkController.isLoggedIn) {
                FBLinkController.fetchSubscription()
            }
        }
    }

    readonly property string apiBase: "https://srv.frakebit.com"

    readonly property var plans: [
        {
            id: "basic",
            title: qsTr("Premium"),
            heroTitle: qsTr("Premium"),
            heroSubtitle: qsTr("Стабильная защита для ежедневной работы"),
            price: "199 ₽",
            period: qsTr("/ 30 дней"),
            badge: qsTr("ОСНОВА"),
            saving: qsTr("Надёжный AWG и все базовые функции безопасности без ограничений трафика"),
            cta: qsTr("Выбрать Premium — 199 ₽"),
            features: [
                { icon: "qrc:/images/controls/gauge.svg",       text: qsTr("Стабильный AWG-протокол с низкой задержкой") },
                { icon: "qrc:/images/controls/shield-tick.svg", text: qsTr("Безлимитный трафик и автопереподключение") },
                { icon: "qrc:/images/controls/map-pin.svg",     text: qsTr("Серверы в 10+ странах и регионах") },
                { icon: "qrc:/images/controls/monitor.svg",     text: qsTr("До 5 устройств в одном аккаунте") },
                { icon: "qrc:/images/controls/info.svg",        text: qsTr("Kill Switch и базовая защита DNS") }
            ]
        },
        {
            id: "vip",
            title: qsTr("VIP"),
            heroTitle: qsTr("VIP"),
            heroSubtitle: qsTr("Максимальный контроль трафика и приоритетная сеть"),
            price: "399 ₽",
            period: qsTr("/ 30 дней"),
            badge: qsTr("МАКС"),
            saving: qsTr("XRay VLESS, VIP-маршрутизация и AdBlock DNS в одном плане"),
            cta: qsTr("Выбрать VIP — 399 ₽"),
            features: [
                { icon: "qrc:/images/controls/gauge.svg",       text: qsTr("XRay VLESS (Reality) для сложных сетей и DPI") },
                { icon: "qrc:/images/controls/split-tunneling.svg", text: qsTr("Профили маршрутизации direct/proxy по сервисам") },
                { icon: "qrc:/images/controls/server.svg",      text: qsTr("Раздельное туннелирование сайтов и приложений") },
                { icon: "qrc:/images/controls/shield.svg",      text: qsTr("AdBlock DNS профиль для VIP-трафика") },
                { icon: "qrc:/images/controls/crown.svg",       text: qsTr("Все преимущества Premium и приоритетный маршрут") }
            ]
        }
    ]

    function planTitleById(planId) {
        for (var i = 0; i < root.plans.length; i++) {
            if (root.plans[i].id === planId) {
                return root.plans[i].title
            }
        }
        if (planId === "trial") return qsTr("Premium")
        return qsTr("Подписка")
    }

    Flickable {
        anchors.fill: parent
        contentHeight: mainColumn.implicitHeight + 32
        clip: true

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: mainColumn
            width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
            x: (parent.width - width) / 2
            spacing: 0

            // Back button
            BackButtonType {
                Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
                Layout.leftMargin: 4
            }

            // ── Hero section ──────────────────────────────────────
            PremiumPanel {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                padding: root.wideLayout ? 20 : 16
                radius: 18
                fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                accentVisible: false

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    PremiumBadge { text: qsTr("ПОДПИСКА"); tone: "neutral"; compact: true }
                    PremiumBadge { text: root.selectedPlanData.id === "vip" ? qsTr("ПРИОРИТЕТ") : qsTr("СТАНДАРТ"); tone: root.selectedPlanData.id === "vip" ? "success" : "warning"; compact: true }
                    Item { Layout.fillWidth: true }
                    PremiumBadge {
                        text: (root.showNewUserOffer && root.selectedPlanData.id === "basic")
                            ? qsTr("5 ₽ / 7 дней")
                            : (root.selectedPlanData.price + " " + root.selectedPlanData.period)
                        tone: root.selectedPlanData.id === "vip" ? "success" : "warning"
                        compact: true
                    }
                }

                LabelTextType {
                    Layout.fillWidth: true
                    text: qsTr("FBLink VPN %1").arg(root.selectedPlanData.heroTitle)
                    font.pixelSize: root.wideLayout ? 28 : 24
                    font.weight: 700
                    color: FBLinkStyle.color.paleGray
                    wrapMode: Text.WordWrap
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    text: (root.showNewUserOffer && root.selectedPlanData.id === "basic")
                        ? qsTr("Для новых пользователей: 7 дней Premium за 5 ₽, затем стандартный тариф")
                        : root.selectedPlanData.heroSubtitle
                    color: FBLinkStyle.color.mutedGray
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                // intentionally minimal hero: badges + title + subtitle only
            }

            // ── Trial banner ──────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: false
                implicitHeight: trialCol.implicitHeight + 24
                radius: 16

                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(16/255, 185/255, 129/255, 0.22) }
                    GradientStop { position: 1.0; color: Qt.rgba(28/255, 29/255, 33/255, 1.0) }
                }
                border.color: Qt.rgba(16/255, 185/255, 129/255, 0.5)
                border.width: 2

                ColumnLayout {
                    id: trialCol
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: 20; anchors.rightMargin: 20
                    spacing: 10

                    RowLayout {
                        spacing: 10

                        // Spark icon
                        Rectangle {
                            width: 36; height: 36; radius: 10
                            color: Qt.rgba(16/255, 185/255, 129/255, 0.2)

                            LabelTextType {
                                anchors.centerIn: parent
                                text: "✦"
                                font.pixelSize: 18
                                color: "#10B981"
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Layout.fillWidth: true

                            LabelTextType {
                                text: qsTr("Пробный период — 7 дней")
                                font.pixelSize: 15
                                font.weight: 700
                                color: "#10B981"
                            }

                            LabelTextType {
                                text: qsTr("Premium на 7 дней с полным доступом")
                                font.pixelSize: 12
                                color: FBLinkStyle.color.mutedGray
                            }
                        }

                        // Price badge
                        Rectangle {
                            height: 32
                            width: priceLabel.implicitWidth + 16
                            radius: 10
                            color: "#10B981"

                            Text {
                                id: priceLabel
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                text: "5 ₽"
                                font.pixelSize: 14
                                font.weight: 700
                                font.family: "PT Root UI VF"
                                color: "white"
                                wrapMode: Text.NoWrap
                            }
                        }
                    }

                    // CTA button
                    Rectangle {
                        Layout.fillWidth: true
                        height: 44
                        radius: 12
                        color: trialBtnMouse.pressed
                            ? Qt.rgba(16/255, 185/255, 129/255, 0.7)
                            : Qt.rgba(16/255, 185/255, 129/255, 0.9)

                        LabelTextType {
                            anchors.centerIn: parent
                            text: root.isLoading
                                ? qsTr("Создание платежа...")
                                : qsTr("Попробовать за 5 ₽")
                            font.pixelSize: 14
                            font.weight: 700
                            color: "white"
                        }

                        MouseArea {
                            id: trialBtnMouse
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            enabled: !root.isLoading
                            onClicked: {
                                root.errorMessage = ""
                                if (!FBLinkController.isLoggedIn) {
                                    PageController.goToPage(PageEnum.PageFBLinkLogin)
                                    return
                                }
                                root.isLoading = true
                                PageController.showBusyIndicator(true)
                                FBLinkController.createPayment("trial")
                            }
                        }
                    }

                    LabelTextType {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Только для новых пользователей • Карта сохраняется")
                        font.pixelSize: 10
                        color: FBLinkStyle.color.mutedGray
                        Layout.bottomMargin: 2
                    }
                }
            }

            // ── Plan cards ────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 12

                Repeater {
                    model: root.plans

                    delegate: Rectangle {
                        id: planCard
                        Layout.fillWidth: true
                        implicitHeight: cardContent.implicitHeight + 32
                        radius: 16
                        clip: true

                        property bool isSelected: root.selectedPlan === index
                        property bool isCurrentPlan: FBLinkController.isSubscribed && FBLinkController.subscriptionPlan === modelData.id
                        property bool isBlocked: FBLinkController.isSubscribed && index <= root.currentPlanLevel
                        property bool isNewUserDiscountCard: root.showNewUserOffer && modelData.id === "basic"

                        // Gradient fill
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: planCard.isCurrentPlan
                                    ? Qt.rgba(16/255, 185/255, 129/255, 0.18)
                                    : (planCard.isSelected
                                        ? (planCard.isNewUserDiscountCard
                                            ? Qt.rgba(16/255, 185/255, 129/255, 0.22)
                                            : Qt.rgba(234/255, 179/255, 8/255, 0.22))
                                        : Qt.rgba(36/255, 36/255, 42/255, 1.0))
                            }
                            GradientStop {
                                position: 1.0
                                color: Qt.rgba(28/255, 29/255, 33/255, 1.0)
                            }
                        }

                        border.width: (planCard.isSelected || planCard.isCurrentPlan) ? 2 : 1
                        border.color: planCard.isCurrentPlan
                            ? "#10B981"
                            : (planCard.isSelected
                                ? (planCard.isNewUserDiscountCard ? "#10B981" : "#EAB308")
                                : FBLinkStyle.color.slateGray)

                        opacity: (planCard.isBlocked && !planCard.isCurrentPlan) ? 0.52 : 1.0

                        Behavior on border.width { NumberAnimation { duration: 150 } }

                        ColumnLayout {
                            id: cardContent
                            anchors {
                                left: parent.left
                                right: parent.right
                                top: parent.top
                                bottom: parent.bottom
                            }
                            anchors.leftMargin: 20
                            anchors.rightMargin: 20
                            anchors.topMargin: 16
                            anchors.bottomMargin: 16
                            spacing: 8

                            Item {
                                Layout.fillWidth: true
                                implicitHeight: Math.max(planMeta.implicitHeight, 22)

                                Rectangle {
                                    id: selectionIndicator
                                    width: 22; height: 22
                                    radius: 11
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    border.width: (planCard.isSelected || planCard.isCurrentPlan) ? 0 : 2
                                    border.color: FBLinkStyle.color.slateGray
                                    color: planCard.isCurrentPlan
                                        ? "#10B981"
                                        : (planCard.isSelected
                                            ? (planCard.isNewUserDiscountCard ? "#10B981" : "#EAB308")
                                            : "transparent")

                                    Behavior on color { ColorAnimation { duration: 150 } }

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 8; height: 8
                                        radius: 4
                                        color: "white"
                                        visible: planCard.isSelected || planCard.isCurrentPlan
                                    }
                                }

                                Row {
                                    id: planMeta
                                    anchors.left: parent.left
                                    anchors.right: selectionIndicator.left
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    LabelTextType {
                                        id: planTitleLabel
                                        width: Math.max(0, planMeta.width - (activeBadge.visible ? activeBadge.width + planMeta.spacing : 0))
                                        text: planCard.isNewUserDiscountCard
                                            ? qsTr("Premium для новых пользователей")
                                            : modelData.title
                                        font.pixelSize: 15
                                        font.weight: 600
                                        color: planCard.isSelected ? FBLinkStyle.color.paleGray : FBLinkStyle.color.lightGray
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    // Badge: "АКТИВНА" overrides normal badge
                                    Rectangle {
                                        id: activeBadge
                                        visible: planCard.isCurrentPlan || modelData.badge !== "" || planCard.isNewUserDiscountCard
                                        height: 20
                                        width: activeBadgeText.implicitWidth + 12
                                        radius: 10
                                        color: planCard.isCurrentPlan ? "#10B981" : (planCard.isNewUserDiscountCard ? "#10B981" : "#EAB308")

                                        Text {
                                            id: activeBadgeText
                                            anchors.fill: parent
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            text: planCard.isCurrentPlan
                                                ? qsTr("АКТИВНА")
                                                : (planCard.isNewUserDiscountCard ? qsTr("НОВЫЙ") : modelData.badge)
                                            font.pixelSize: 10
                                            font.weight: 700
                                            font.family: "PT Root UI VF"
                                            color: "#FFFFFF"
                                            wrapMode: Text.NoWrap
                                        }
                                    }
                                }
                            }

                            // Price row
                            RowLayout {
                                spacing: 4

                                LabelTextType {
                                    text: planCard.isNewUserDiscountCard ? qsTr("5 ₽") : modelData.price
                                    font.pixelSize: 26
                                    font.weight: 700
                                    color: planCard.isSelected
                                        ? (planCard.isNewUserDiscountCard ? "#10B981" : "#EAB308")
                                        : FBLinkStyle.color.mutedGray
                                }

                                LabelTextType {
                                    text: planCard.isNewUserDiscountCard ? qsTr("/ 7 дней") : modelData.period
                                    font.pixelSize: 14
                                    color: FBLinkStyle.color.mutedGray
                                    Layout.alignment: Qt.AlignBottom
                                    Layout.bottomMargin: 3
                                }
                            }

                            LabelTextType {
                                visible: planCard.isNewUserDiscountCard
                                text: qsTr("Затем 199 ₽ / 30 дней")
                                font.pixelSize: 12
                                color: FBLinkStyle.color.mutedGray
                            }

                            // Saving hint
                            LabelTextType {
                                visible: modelData.saving !== ""
                                text: planCard.isNewUserDiscountCard
                                    ? qsTr("Специальный запуск: полный Premium-доступ по сниженной цене на 7 дней")
                                    : modelData.saving
                                font.pixelSize: 12
                                color: "#10B981"
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: planCard.isBlocked ? Qt.ForbiddenCursor : Qt.PointingHandCursor
                            onClicked: if (!planCard.isBlocked) root.selectedPlan = index
                        }
                    }
                }
            }

            // ── Features ──────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 28
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 0

                LabelTextType {
                    text: qsTr("Преимущества тарифа")
                    font.pixelSize: 13
                    font.weight: 600
                    color: FBLinkStyle.color.mutedGray
                    Layout.bottomMargin: 12
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 14
                    radius: 14
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: false

                    Repeater {
                        model: root.selectedPlanData.features

                        delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Layout.bottomMargin: 10

                        // Check circle
                        Rectangle {
                            width: 28; height: 28
                            radius: 14
                            color: Qt.rgba(16/255, 185/255, 129/255, 0.15)

                            Image {
                                anchors.centerIn: parent
                                source: modelData.icon || "qrc:/images/controls/shield-tick.svg"
                                sourceSize: Qt.size(14, 14)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#10B981" }
                            }
                        }

                        LabelTextType {
                            text: modelData.text
                            font.pixelSize: 14
                            color: FBLinkStyle.color.lightGray
                            Layout.fillWidth: true
                        }
                    }
                }
                }
            }

            // ── Error message ─────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                height: errText.implicitHeight + 16
                color: "#3D1515"
                radius: 8
                visible: root.errorMessage !== ""

                LabelTextType {
                    id: errText
                    anchors.centerIn: parent
                    width: parent.width - 24
                    text: root.errorMessage
                    color: "#FF6B6B"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 13
                }
            }

            // ── Waiting for payment confirmation ──────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: root.isWaitingForPayment
                color: Qt.rgba(234/255, 179/255, 8/255, 0.1)
                border.color: Qt.rgba(234/255, 179/255, 8/255, 0.35)
                border.width: 1
                radius: 12
                implicitHeight: waitingCol.implicitHeight + 24

                ColumnLayout {
                    id: waitingCol
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: 16; anchors.rightMargin: 16
                    spacing: 6

                    RowLayout {
                        spacing: 8

                        // Spinner dot animation
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: "#EAB308"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                PropertyAnimation { to: 0.2; duration: 600 }
                                PropertyAnimation { to: 1.0; duration: 600 }
                            }
                        }

                        LabelTextType {
                            text: qsTr("Ожидаем подтверждение оплаты...")
                            font.pixelSize: 13
                            font.weight: 600
                            color: "#FDE68A"
                        }
                    }

                    LabelTextType {
                        text: qsTr("Проверка %1 из %2. Это займёт до 2 минут.").arg(root.pollCount).arg(root.maxPolls)
                        font.pixelSize: 11
                        color: FBLinkStyle.color.mutedGray
                    }
                }
            }

            // Manual check button (always visible when waiting or after timeout)
            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                implicitHeight: 44
                visible: root.isWaitingForPayment || root.errorMessage !== ""

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(234/255, 179/255, 8/255, 0.12)
                pressedColor: Qt.rgba(234/255, 179/255, 8/255, 0.2)
                textColor: FBLinkStyle.color.paleGray
                borderColor: Qt.rgba(234/255, 179/255, 8/255, 0.5)
                borderWidth: 1

                text: qsTr("Проверить вручную")

                clickedFunc: function() {
                    if (FBLinkController.isLoggedIn) {
                        FBLinkController.fetchSubscription()
                    }
                }
            }

            // ── CTA Button ────────────────────────────────────────
            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: root.errorMessage !== "" ? 12 : 28
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                implicitHeight: 56

                visible: !root.isWaitingForPayment
                enabled: !root.isLoading && root.selectedPlan > root.currentPlanLevel

                defaultColor: root.selectedPlan > root.currentPlanLevel
                    ? (root.selectedPlanData.id === "vip"
                        ? "#10B981"
                        : ((root.showNewUserOffer && root.selectedPlanData.id === "basic") ? "#10B981" : "#EAB308"))
                    : FBLinkStyle.color.charcoalGray
                hoveredColor: (root.selectedPlanData.id === "vip" || (root.showNewUserOffer && root.selectedPlanData.id === "basic"))
                    ? "#34D399"
                    : "#FACC15"
                pressedColor: (root.selectedPlanData.id === "vip" || (root.showNewUserOffer && root.selectedPlanData.id === "basic"))
                    ? "#059669"
                    : "#CA8A04"
                disabledColor: FBLinkStyle.color.charcoalGray
                textColor: "#FFFFFF"

                text: root.isLoading
                    ? qsTr("Создание платежа...")
                    : (root.selectedPlan <= root.currentPlanLevel
                        ? qsTr("Уже активна")
                        : ((root.showNewUserOffer && root.selectedPlanData.id === "basic")
                            ? qsTr("Активировать 7 дней за 5 ₽")
                            : root.selectedPlanData.cta))

                clickedFunc: function() {
                    root.errorMessage = ""

                    if (!FBLinkController.isLoggedIn) {
                        PageController.goToPage(PageEnum.PageFBLinkLogin)
                        return
                    }

                    root.isLoading = true
                    PageController.showBusyIndicator(true)
                    var selectedPlanId = root.plans[root.selectedPlan].id
                    if (root.showNewUserOffer && selectedPlanId === "basic") {
                        selectedPlanId = "trial"
                    }
                    FBLinkController.createPayment(selectedPlanId)
                }
            }

            // Connections handle payment result from C++
            Connections {
                target: FBLinkController

                function onPaymentCreated(confirmationUrl) {
                    root.isLoading = false
                    PageController.showBusyIndicator(false)
                    if (confirmationUrl !== "") {
                        Qt.openUrlExternally(confirmationUrl)
                        root.isWaitingForPayment = true
                        root.pollCount = 0
                        pollTimer.start()
                        PageController.showNotificationMessage(
                            qsTr("Страница оплаты открыта. Ожидаем подтверждение..."))
                    } else {
                        root.errorMessage = qsTr("Не удалось получить ссылку на оплату")
                    }
                }

                function onPaymentError(errorMessage) {
                    root.isLoading = false
                    PageController.showBusyIndicator(false)
                    root.errorMessage = errorMessage
                }

                function onSubscriptionChanged() {
                    if (FBLinkController.isSubscribed && root.isWaitingForPayment) {
                        pollTimer.stop()
                        root.isWaitingForPayment = false
                        root.selectedPlan = Math.max(root.currentPlanLevel, 0)
                        PageController.showNotificationMessage(qsTr("Подписка активирована! Добро пожаловать в %1.").arg(root.planTitleById(FBLinkController.subscriptionPlan)))
                        FBLinkController.armNewFeaturesGuide()
                        FBLinkController.fetchConfig()
                        PageController.goToPageHome()
                    }
                }
            }

            // Already subscribed — status + management
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8
                spacing: 10
                visible: FBLinkController.isSubscribed

                // ── Active subscription status ────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: activeSubRow.implicitHeight + 20
                    radius: 12
                    color: Qt.rgba(16/255, 185/255, 129/255, 0.1)
                    border.color: Qt.rgba(16/255, 185/255, 129/255, 0.3)
                    border.width: 1

                    RowLayout {
                        id: activeSubRow
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 12

                        Image {
                            source: "qrc:/images/controls/shield-tick.svg"
                            sourceSize: Qt.size(20, 20)
                            layer.enabled: true
                            layer.effect: ColorOverlay { color: "#10B981" }
                        }

                        ColumnLayout {
                            spacing: 1
                            Layout.fillWidth: true

                            LabelTextType {
                        text: qsTr("Подписка «%1» активна").arg(root.planTitleById(FBLinkController.subscriptionPlan))
                                font.pixelSize: 13
                                font.weight: 600
                                color: "#10B981"
                            }

                            LabelTextType {
                                text: qsTr("Действует до: ") + new Date(FBLinkController.subscriptionEndDate).toLocaleDateString(Qt.locale(), Locale.LongFormat)
                                font.pixelSize: 12
                                color: FBLinkStyle.color.mutedGray
                            }
                        }
                    }
                }

                // ── Management panel ──────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: mgmtCol.implicitHeight + 28
                    radius: 14
                    color: Qt.rgba(36/255, 36/255, 42/255, 1.0)
                    border.color: Qt.rgba(255, 255, 255, 0.07)
                    border.width: 1

                    ColumnLayout {
                        id: mgmtCol
                        anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: 14 }
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 0

                        // Auto-renew row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                spacing: 3
                                Layout.fillWidth: true

                                LabelTextType {
                                    text: qsTr("Автопродление")
                                    font.pixelSize: 14
                                    font.weight: 600
                                    color: FBLinkStyle.color.paleGray
                                }

                                LabelTextType {
                                    text: FBLinkController.autoRenew
                                        ? qsTr("Спишем автоматически в день истечения")
                                        : qsTr("Подписка не продлится сама")
                                    font.pixelSize: 11
                                    color: FBLinkStyle.color.mutedGray
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }

                            // Toggle
                            Rectangle {
                                width: 46; height: 26; radius: 13
                                color: FBLinkController.autoRenew
                                    ? "#10B981"
                                    : Qt.rgba(255, 255, 255, 0.12)
                                Behavior on color { ColorAnimation { duration: 180 } }

                                Rectangle {
                                    width: 20; height: 20; radius: 10
                                    color: "white"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: FBLinkController.autoRenew ? parent.width - width - 3 : 3
                                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: FBLinkController.setAutoRenew(!FBLinkController.autoRenew)
                                }
                            }
                        }

                        // Divider
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Qt.rgba(255, 255, 255, 0.07)
                            Layout.topMargin: 14
                            Layout.bottomMargin: 14
                        }

                        // Card row
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 0
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 10
                                color: FBLinkController.cardSaved
                                    ? Qt.rgba(16/255, 185/255, 129/255, 0.15)
                                    : Qt.rgba(255, 255, 255, 0.06)

                                Image {
                                    anchors.centerIn: parent
                                    source: "qrc:/images/controls/info.svg"
                                    sourceSize: Qt.size(18, 18)
                                    layer.enabled: true
                                    layer.effect: ColorOverlay {
                                        color: FBLinkController.cardSaved
                                            ? "#10B981"
                                            : FBLinkStyle.color.mutedGray
                                    }
                                }
                            }

                            ColumnLayout {
                                spacing: 3
                                Layout.fillWidth: true

                                LabelTextType {
                                    text: qsTr("Способ оплаты")
                                    font.pixelSize: 14
                                    font.weight: 600
                                    color: FBLinkStyle.color.paleGray
                                }

                                LabelTextType {
                                    text: FBLinkController.cardSaved
                                        ? qsTr("Карта привязана ✓")
                                        : qsTr("Сохранится при следующей оплате")
                                    font.pixelSize: 11
                                    color: FBLinkController.cardSaved
                                        ? "#10B981"
                                        : FBLinkStyle.color.mutedGray
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }

                            Rectangle {
                                visible: FBLinkController.cardSaved
                                width: 70; height: 30; radius: 8
                                color: deleteCardMouse.pressed
                                    ? Qt.rgba(239/255, 68/255, 68/255, 0.25)
                                    : Qt.rgba(239/255, 68/255, 68/255, 0.1)
                                border.color: Qt.rgba(239/255, 68/255, 68/255, 0.4)
                                border.width: 1

                                LabelTextType {
                                    anchors.centerIn: parent
                                    text: qsTr("Удалить")
                                    font.pixelSize: 11
                                    font.weight: 600
                                    color: "#EF4444"
                                }

                                MouseArea {
                                    id: deleteCardMouse
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.confirmDeleteCard = true
                                }
                            }
                        }
                    }
                }

                // Confirm delete card dialog
                Rectangle {
                    Layout.fillWidth: true
                    visible: root.confirmDeleteCard
                    implicitHeight: confirmCol.implicitHeight + 24
                    radius: 12
                    color: Qt.rgba(60/255, 20/255, 20/255, 1.0)
                    border.color: Qt.rgba(239/255, 68/255, 68/255, 0.4)
                    border.width: 1

                    ColumnLayout {
                        id: confirmCol
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 12

                        LabelTextType {
                            text: qsTr("Удалить привязанную карту и отключить автосписание?")
                            font.pixelSize: 13
                            font.weight: 600
                            color: "#FF6B6B"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Rectangle {
                                Layout.fillWidth: true
                                height: 36; radius: 8
                                color: cancelMouse.pressed
                                    ? Qt.rgba(255,255,255,0.12)
                                    : Qt.rgba(255,255,255,0.07)
                                border.color: Qt.rgba(255,255,255,0.15)
                                border.width: 1

                                LabelTextType {
                                    anchors.centerIn: parent
                                    text: qsTr("Отмена")
                                    font.pixelSize: 13
                                    color: FBLinkStyle.color.lightGray
                                }
                                MouseArea {
                                    id: cancelMouse
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.confirmDeleteCard = false
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 36; radius: 8
                                color: confirmMouse.pressed
                                    ? Qt.rgba(239/255, 68/255, 68/255, 0.5)
                                    : Qt.rgba(239/255, 68/255, 68/255, 0.25)
                                border.color: Qt.rgba(239/255, 68/255, 68/255, 0.6)
                                border.width: 1

                                LabelTextType {
                                    anchors.centerIn: parent
                                    text: qsTr("Удалить")
                                    font.pixelSize: 13
                                    font.weight: 600
                                    color: "#EF4444"
                                }
                                MouseArea {
                                    id: confirmMouse
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.confirmDeleteCard = false
                                        FBLinkController.deleteCard()
                                    }
                                }
                            }
                        }
                    }
                }

                // Error message for management actions
                Rectangle {
                    Layout.fillWidth: true
                    visible: root.mgmtError !== ""
                    height: mgmtErrText.implicitHeight + 16
                    color: "#3D1515"
                    radius: 8

                    LabelTextType {
                        id: mgmtErrText
                        anchors.centerIn: parent
                        width: parent.width - 24
                        text: root.mgmtError
                        color: "#FF6B6B"
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                    }
                }

                // Connections for management actions
                Connections {
                    target: FBLinkController

                    function onCardDeleted() {
                        root.mgmtError = ""
                        PageController.showNotificationMessage(qsTr("Карта удалена, автосписание отключено"))
                    }

                    function onAutoRenewChanged(enabled) {
                        root.mgmtError = ""
                    }

                    function onRequestError(errorMessage) {
                        root.mgmtError = errorMessage
                    }
                }
            }

            // ── Footer links ──────────────────────────────────────
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 16
                Layout.bottomMargin: 24 + SettingsController.safeAreaBottomMargin
                spacing: 4

                ButtonTextType {
                    text: qsTr("Условия использования")
                    font.pixelSize: 12
                    color: FBLinkStyle.color.mutedGray

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://fblink.com/terms")
                    }
                }

                LabelTextType {
                    text: "·"
                    color: FBLinkStyle.color.charcoalGray
                    font.pixelSize: 12
                }

                ButtonTextType {
                    text: qsTr("Политика конфиденциальности")
                    font.pixelSize: 12
                    color: FBLinkStyle.color.mutedGray

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://fblink.com/privacy")
                    }
                }
            }
        }
    }

    // Refresh subscription status when page opens
    Component.onCompleted: {
        root.selectedPlan = Math.max(root.currentPlanLevel, 0)
        if (FBLinkController.isLoggedIn) {
            FBLinkController.fetchSubscription()
        }
    }
}
