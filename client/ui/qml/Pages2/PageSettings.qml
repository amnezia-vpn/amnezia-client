import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    function goTo(page) {
        PageController.goToPage(page)
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

                BackButtonType {
                    Layout.topMargin: 16 + SettingsController.safeAreaTopMargin
                    Layout.leftMargin: 4
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: FBLinkController.isSubscribed ? "#10B981" : "#00C8FF"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                LabelTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Настройки")
                                    font.pixelSize: root.wideLayout ? 24 : 21
                                    font.weight: 700
                                    color: FBLinkStyle.color.paleGray
                                    wrapMode: Text.WordWrap
                                }

                                PremiumBadge {
                                    text: root.accountTierLabel()
                                    tone: FBLinkController.isSubscribed ? "success" : "accent"
                                    iconSource: "qrc:/images/controls/shield-tick.svg"
                                }
                            }

                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        width: parent ? parent.width : 0
                        spacing: 8

                        PremiumBadge {
                            visible: FBLinkController.isLoggedIn
                            text: root.formatSubscriptionDate()
                            tone: FBLinkController.isSubscribed ? "success" : "neutral"
                            iconSource: "qrc:/images/controls/history.svg"
                        }

                        PremiumBadge {
                            text: FBLinkController.isLoggedIn ? qsTr("Аккаунт подключён") : qsTr("Гость")
                            tone: FBLinkController.isLoggedIn ? "accent" : "neutral"
                            iconSource: FBLinkController.isLoggedIn
                                ? "qrc:/images/controls/check.svg"
                                : "qrc:/images/controls/info.svg"
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.wideLayout ? 2 : 1
                    columnSpacing: 14
                    rowSpacing: 14

                    PremiumPanel {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        padding: 12
                        accentVisible: true
                        accentColor: "#00C8FF"

                        LabelTextType {
                            text: qsTr("Аккаунт")
                            font.pixelSize: 17
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            text: FBLinkController.isLoggedIn ? qsTr("Подписка") : qsTr("Войти")
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            textColor: "#FFFFFF"
                            leftImageSource: "qrc:/images/controls/shield-tick.svg"
                            clickedFunc: function() {
                                if (FBLinkController.isLoggedIn) {
                                    root.goTo(PageEnum.PageFBLinkSubscription)
                                } else {
                                    root.goTo(PageEnum.PageFBLinkLogin)
                                }
                            }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            visible: FBLinkController.isLoggedIn
                            text: qsTr("VIP-пресеты")
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            leftImageSource: "qrc:/images/controls/tag.svg"
                            clickedFunc: function() {
                                root.goTo(PageEnum.PageSettingsVipRoutingProfiles)
                            }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            visible: FBLinkController.isLoggedIn
                            text: qsTr("Выйти")
                            defaultColor: Qt.rgba(239/255, 68/255, 68/255, 0.14)
                            hoveredColor: Qt.rgba(239/255, 68/255, 68/255, 0.20)
                            pressedColor: Qt.rgba(239/255, 68/255, 68/255, 0.28)
                            textColor: "#EF4444"
                            leftImageSource: "qrc:/images/controls/x-circle.svg"
                            clickedFunc: function() {
                                FBLinkController.logout()
                                PageController.goToPageHome()
                            }
                        }
                    }

                    PremiumPanel {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        padding: 12
                        accentVisible: true
                        accentColor: "#10B981"

                        LabelTextType {
                            text: qsTr("Подключение")
                            font.pixelSize: 17
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            text: qsTr("Подключение")
                            leftImageSource: "qrc:/images/controls/radio.svg"
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: function() { root.goTo(PageEnum.PageSettingsConnection) }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            text: qsTr("Приложение")
                            leftImageSource: "qrc:/images/controls/app.svg"
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: function() { root.goTo(PageEnum.PageSettingsApplication) }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            visible: ServersModel.hasServersFromGatewayApi
                            text: NewsModel.hasUnread && SettingsController.isNewsNotificationsEnabled()
                                ? qsTr("Новости • новое")
                                : qsTr("Новости")
                            leftImageSource: NewsModel.hasUnread && SettingsController.isNewsNotificationsEnabled()
                                ? "qrc:/images/controls/news-unread.svg"
                                : "qrc:/images/controls/news.svg"
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: function() {
                                if (!ServersModel.hasServersFromGatewayApi) {
                                    return
                                }
                                PageController.showBusyIndicator(true)
                                ApiNewsController.fetchNews(true)
                                root.goTo(PageEnum.PageSettingsNewsNotifications)
                            }
                        }
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: "#F59E0B"

                    LabelTextType {
                        text: qsTr("Инструменты")
                        font.pixelSize: 17
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                    }

                    Flow {
                        Layout.fillWidth: true
                        width: parent ? parent.width : 0
                        spacing: 10

                        BasicButtonType {
                            width: root.wideLayout ? 220 : parent.width
                            implicitHeight: 44
                            text: qsTr("О приложении")
                            leftImageSource: "qrc:/images/controls/info.svg"
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: function() { root.goTo(PageEnum.PageSettingsAbout) }
                        }

                        BasicButtonType {
                            visible: SettingsController.isDevModeEnabled
                            width: root.wideLayout ? 220 : parent.width
                            implicitHeight: 44
                            text: qsTr("Dev-консоль")
                            leftImageSource: "qrc:/images/controls/bug.svg"
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: function() { root.goTo(PageEnum.PageDevMenu) }
                        }
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
