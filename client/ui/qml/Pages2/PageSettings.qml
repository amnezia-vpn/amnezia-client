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

    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    property bool adBlockSwitchSyncing: false

    function goTo(page) {
        PageController.goToPage(page)
    }

    function syncAdBlockSwitchFromState() {
        if (!adBlockSwitch) {
            return
        }
        root.adBlockSwitchSyncing = true
        adBlockSwitch.checked = FBLinkController.vipAdBlockEnabled
        root.adBlockSwitchSyncing = false
    }

    function formatSubscriptionDate() {
        if (!FBLinkController.isSubscribed) {
            return qsTr("Без активной подписки")
        }
        return qsTr("Действует до %1").arg(new Date(FBLinkController.subscriptionEndDate).toLocaleDateString(Qt.locale(), Locale.LongFormat))
    }

    function accountTierLabel() {
        if (!FBLinkController.isSubscribed) {
            return qsTr("Free")
        }
        return FBLinkController.subscriptionPlan === "vip" ? qsTr("VIP") : qsTr("Premium")
    }

    Connections {
        target: ApiNewsController

        function onFetchNewsFinished() {
            PageController.showBusyIndicator(false)
        }

        function onErrorOccurred(errorCode, showError) {
            if (showError) {
                PageController.showErrorMessage(errorCode)
                PageController.closePage()
                PageController.showBusyIndicator(false)
            }
        }
    }

    Connections {
        target: FBLinkController

        function onVipAdBlockChanged(enabled) {
            root.syncAdBlockSwitchFromState()
        }

        function onSubscriptionChanged() {
            root.syncAdBlockSwitchFromState()
        }
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight + 28

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        Item {
            width: parent.width
            height: content.implicitHeight + 28

            ColumnLayout {
                id: content
                width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                spacing: 10

                LabelTextType {
                    Layout.topMargin: 16 + SettingsController.safeAreaTopMargin
                    Layout.fillWidth: true
                    text: qsTr("Настройки")
                    font.pixelSize: root.wideLayout ? 30 : 26
                    font.weight: 700
                    color: FBLinkStyle.color.paleGray
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 92
                    radius: 16
                    color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 46
                            radius: 23
                            color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                            border.width: 1
                            border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/mail.svg"
                                sourceSize: Qt.size(22, 22)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                LabelTextType {
                                    text: qsTr("Аккаунт FBLink")
                                    color: FBLinkStyle.color.paleGray
                                    font.pixelSize: 15
                                    font.weight: 700
                                }

                                Rectangle {
                                    visible: FBLinkController.isSubscribed
                                    implicitWidth: tierRow.implicitWidth + 12
                                    implicitHeight: tierText.implicitHeight + 4
                                    radius: 6
                                    color: FBLinkController.subscriptionPlan === "vip"
                                        ? Qt.rgba(234/255, 179/255, 8/255, 0.12)
                                        : Qt.rgba(59/255, 130/255, 246/255, 0.12)
                                    border.width: 1
                                    border.color: FBLinkController.subscriptionPlan === "vip"
                                        ? Qt.rgba(234/255, 179/255, 8/255, 0.25)
                                        : Qt.rgba(59/255, 130/255, 246/255, 0.25)

                                    RowLayout {
                                        id: tierRow
                                        anchors.centerIn: parent
                                        spacing: 4

                                        Image {
                                            visible: FBLinkController.subscriptionPlan === "vip"
                                            source: "qrc:/images/controls/crown.svg"
                                            sourceSize: Qt.size(12, 12)
                                            layer.enabled: true
                                            layer.effect: ColorOverlay { color: "#EAB308" }
                                        }

                                        CaptionTextType {
                                            id: tierText
                                            text: FBLinkController.subscriptionPlan === "vip" ? qsTr("VIP") : root.accountTierLabel()
                                            color: FBLinkController.subscriptionPlan === "vip" ? "#EAB308" : "#60A5FA"
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                }
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: FBLinkController.isLoggedIn
                                    ? (FBLinkController.userEmail !== "" ? FBLinkController.userEmail : qsTr("Email не получен"))
                                    : qsTr("Гость")
                                color: FBLinkStyle.color.mutedGray
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            visible: FBLinkController.isLoggedIn
                            radius: 12
                            color: logoutMouse.pressed
                                ? Qt.rgba(20/255, 20/255, 20/255, 1.0)
                                : (logoutMouse.containsMouse ? Qt.rgba(24/255, 24/255, 24/255, 1.0) : Qt.rgba(12/255, 12/255, 12/255, 1.0))
                            border.width: 1
                            border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/log-out.svg"
                                sourceSize: Qt.size(22, 22)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#EF4444" }
                            }

                            MouseArea {
                                id: logoutMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    FBLinkController.logout()
                                    PageController.goToPageHome()
                                }
                            }
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    text: qsTr("ПОДКЛЮЧЕНИЕ")
                    color: FBLinkStyle.color.mutedGray
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 16
                    color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    implicitHeight: 234

                    Column {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            width: parent.width
                            height: 78
                            color: "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                                    border.width: 1
                                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/images/controls/shield-tick.svg"
                                        sourceSize: Qt.size(18, 18)
                                        layer.enabled: true
                                        layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    LabelTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Kill Switch")
                                        color: FBLinkStyle.color.paleGray
                                        font.pixelSize: 14
                                        font.weight: 600
                                    }

                                    CaptionTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Блокировать интернет при обрыве VPN")
                                        color: FBLinkStyle.color.mutedGray
                                    }
                                }

                                SwitcherType {
                                    checked: SettingsController.isKillSwitchEnabled
                                    onToggled: {
                                        if (checked !== SettingsController.isKillSwitchEnabled) {
                                            SettingsController.isKillSwitchEnabled = checked
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Qt.rgba(63/255, 63/255, 70/255, 0.6)
                        }

                        Rectangle {
                            width: parent.width
                            height: 77
                            color: "transparent"
                            opacity: FBLinkController.canManageRoutingProfiles ? 1.0 : 0.45

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                                    border.width: 1
                                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/images/controls/split-tunneling.svg"
                                        sourceSize: Qt.size(18, 18)
                                        layer.enabled: true
                                        layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Image {
                                            Layout.preferredWidth: 14
                                            Layout.preferredHeight: 14
                                            source: "qrc:/images/controls/crown.svg"
                                            sourceSize: Qt.size(14, 14)
                                            layer.enabled: true
                                            layer.effect: ColorOverlay { color: "#F5C542" }
                                        }

                                        LabelTextType {
                                            Layout.fillWidth: true
                                            text: qsTr("Профили маршрутизации")
                                            color: FBLinkStyle.color.paleGray
                                            font.pixelSize: 14
                                            font.weight: 600
                                            elide: Text.ElideRight
                                        }
                                    }

                                    CaptionTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Управление split tunneling и маршрутами")
                                        color: FBLinkStyle.color.mutedGray
                                    }
                                }

                                Image {
                                    source: "qrc:/images/controls/chevron-right.svg"
                                    sourceSize: Qt.size(18, 18)
                                    layer.enabled: true
                                    layer.effect: ColorOverlay { color: FBLinkStyle.color.charcoalGray }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: FBLinkController.canManageRoutingProfiles
                                hoverEnabled: true
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: root.goTo(PageEnum.PageSettingsVipRoutingProfiles)
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Qt.rgba(63/255, 63/255, 70/255, 0.6)
                        }

                        Rectangle {
                            width: parent.width
                            height: 77
                            color: "transparent"
                            opacity: FBLinkController.canUseAdBlock ? 1.0 : 0.5

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                                    border.width: 1
                                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/images/controls/bug.svg"
                                        sourceSize: Qt.size(18, 18)
                                        layer.enabled: true
                                        layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Image {
                                            Layout.preferredWidth: 14
                                            Layout.preferredHeight: 14
                                            source: "qrc:/images/controls/crown.svg"
                                            sourceSize: Qt.size(14, 14)
                                            layer.enabled: true
                                            layer.effect: ColorOverlay { color: "#F5C542" }
                                        }

                                        LabelTextType {
                                            Layout.fillWidth: true
                                            text: qsTr("AdBlock")
                                            color: FBLinkStyle.color.paleGray
                                            font.pixelSize: 14
                                            font.weight: 600
                                            elide: Text.ElideRight
                                        }
                                    }

                                    CaptionTextType {
                                        Layout.fillWidth: true
                                        text: !FBLinkController.canUseAdBlock
                                            ? qsTr("Доступно только для VIP аккаунта")
                                            : (FBLinkController.vipAdBlockEnabled
                                                ? ((FBLinkController.vipAdBlockStatus === "degraded"
                                                    || FBLinkController.vipAdBlockStatus === "unavailable")
                                                    ? qsTr("Фильтрация временно недоступна")
                                                    : qsTr("Блокировка рекламы активна"))
                                                : qsTr("AdBlock выключен"))
                                        color: FBLinkStyle.color.mutedGray
                                    }
                                }

                                SwitcherType {
                                    id: adBlockSwitch
                                    enabled: FBLinkController.canUseAdBlock && !FBLinkController.isLoading
                                    checked: false
                                    Component.onCompleted: root.syncAdBlockSwitchFromState()
                                    onToggled: {
                                        if (root.adBlockSwitchSyncing) {
                                            return
                                        }
                                        if (checked === FBLinkController.vipAdBlockEnabled) {
                                            return
                                        }
                                        FBLinkController.setVipAdBlockEnabled(checked)
                                    }
                                }
                            }
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    text: qsTr("АККАУНТ")
                    color: FBLinkStyle.color.mutedGray
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 16
                    color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    implicitHeight: 78

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            radius: 10
                            color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                            border.width: 1
                            border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/tag.svg"
                                sourceSize: Qt.size(18, 18)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            LabelTextType {
                                Layout.fillWidth: true
                                text: FBLinkController.isLoggedIn ? qsTr("Подписка") : qsTr("Войти")
                                color: FBLinkStyle.color.paleGray
                                font.pixelSize: 14
                                font.weight: 600
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: FBLinkController.isLoggedIn ? qsTr("Управление подпиской") : qsTr("Авторизуйтесь в FBLink ID")
                                color: FBLinkStyle.color.mutedGray
                            }
                        }

                        Image {
                            source: "qrc:/images/controls/chevron-right.svg"
                            sourceSize: Qt.size(18, 18)
                            layer.enabled: true
                            layer.effect: ColorOverlay { color: FBLinkStyle.color.charcoalGray }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (FBLinkController.isLoggedIn) {
                                root.goTo(PageEnum.PageFBLinkSubscription)
                            } else {
                                root.goTo(PageEnum.PageFBLinkLogin)
                            }
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    text: qsTr("ПРИЛОЖЕНИЕ")
                    color: FBLinkStyle.color.mutedGray
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 16
                    color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    implicitHeight: 156

                    Column {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            width: parent.width
                            height: 78
                            color: "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                                    border.width: 1
                                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/images/controls/refresh-cw.svg"
                                        sourceSize: Qt.size(18, 18)
                                        layer.enabled: true
                                        layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    LabelTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Автоподключение")
                                        color: FBLinkStyle.color.paleGray
                                        font.pixelSize: 14
                                        font.weight: 600
                                    }

                                    CaptionTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Подключаться при запуске приложения")
                                        color: FBLinkStyle.color.mutedGray
                                    }
                                }

                                SwitcherType {
                                    checked: SettingsController.isAutoConnectEnabled()
                                    onToggled: {
                                        if (checked !== SettingsController.isAutoConnectEnabled()) {
                                            SettingsController.toggleAutoConnect(checked)
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Qt.rgba(63/255, 63/255, 70/255, 0.6)
                        }

                        Rectangle {
                            width: parent.width
                            height: 77
                            color: "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                                    border.width: 1
                                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                                    Image {
                                        anchors.centerIn: parent
                                        source: "qrc:/images/controls/settings-2.svg"
                                        sourceSize: Qt.size(18, 18)
                                        layer.enabled: true
                                        layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    LabelTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Язык и параметры")
                                        color: FBLinkStyle.color.paleGray
                                        font.pixelSize: 14
                                        font.weight: 600
                                    }

                                    CaptionTextType {
                                        Layout.fillWidth: true
                                        text: qsTr("Дополнительные настройки приложения")
                                        color: FBLinkStyle.color.mutedGray
                                    }
                                }

                                Image {
                                    source: "qrc:/images/controls/chevron-right.svg"
                                    sourceSize: Qt.size(18, 18)
                                    layer.enabled: true
                                    layer.effect: ColorOverlay { color: FBLinkStyle.color.charcoalGray }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.goTo(PageEnum.PageSettingsApplication)
                            }
                        }

                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 16
                    color: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    implicitHeight: 78

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            radius: 10
                            color: Qt.rgba(10/255, 10/255, 10/255, 1.0)
                            border.width: 1
                            border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/info.svg"
                                sourceSize: Qt.size(18, 18)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("О приложении")
                                color: FBLinkStyle.color.paleGray
                                font.pixelSize: 14
                                font.weight: 600
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: qsTr("Версия, лицензии и техническая информация")
                                color: FBLinkStyle.color.mutedGray
                            }
                        }

                        Image {
                            source: "qrc:/images/controls/chevron-right.svg"
                            sourceSize: Qt.size(18, 18)
                            layer.enabled: true
                            layer.effect: ColorOverlay { color: FBLinkStyle.color.charcoalGray }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.goTo(PageEnum.PageSettingsAbout)
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin
                }
            }
        }
    }
}
