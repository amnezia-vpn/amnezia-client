import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

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

    ListViewType {
        id: listView

        anchors.fill: parent

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                id: header
                Layout.fillWidth: true
                Layout.topMargin: 24 + SettingsController.safeAreaTopMargin
                Layout.bottomMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Настройки")
            }
        }

        model: settingsEntries

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: isVisible

                text: title
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: leftImagePath

                clickedFunction: clickedHandler
            }

            DividerType {
                visible: isVisible
            }
        }

        footer: ColumnLayout {
            width: listView.width

            // ── Account section (visible when logged in) ──────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8
                spacing: 8
                visible: FBLinkController.isLoggedIn

                // Account info card
                Rectangle {
                    Layout.fillWidth: true
                    height: accountRow.implicitHeight + 20
                    radius: 12
                    color: Qt.rgba(0, 200/255, 255/255, 0.07)
                    border.color: Qt.rgba(0, 200/255, 255/255, 0.2)
                    border.width: 1

                    RowLayout {
                        id: accountRow
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 10

                        Rectangle {
                            width: 32; height: 32; radius: 16
                            color: Qt.rgba(0, 200/255, 255/255, 0.15)
                            Text { anchors.centerIn: parent; text: "👤"; font.pixelSize: 15 }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: FBLinkController.isSubscribed
                                    ? qsTr("Premium аккаунт")
                                    : qsTr("Бесплатный аккаунт")
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                color: "#FFFFFF"
                            }

                            Text {
                                visible: FBLinkController.isSubscribed
                                text: qsTr("Действует до: ") + new Date(FBLinkController.subscriptionEndDate).toLocaleDateString(Qt.locale(), Locale.LongFormat)
                                font.pixelSize: 11
                                color: Qt.rgba(1, 1, 1, 0.5)
                            }
                        }
                    }
                }

                // Logout button
                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    radius: 12
                    color: logoutMouse.pressed
                        ? Qt.rgba(239/255, 68/255, 68/255, 0.25)
                        : (logoutMouse.containsMouse
                            ? Qt.rgba(239/255, 68/255, 68/255, 0.15)
                            : Qt.rgba(239/255, 68/255, 68/255, 0.08))
                    border.color: Qt.rgba(239/255, 68/255, 68/255, 0.4)
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 120 } }

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "→"
                            font.pixelSize: 16
                            color: "#EF4444"
                        }

                        Text {
                            text: qsTr("Выйти из аккаунта")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: "#EF4444"
                        }
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

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: FBLinkController.isLoggedIn
            }

        }
    }

    property list<QtObject> settingsEntries: [
        connection,
        application,
        news,
        about,
        devConsole
    ]



    QtObject {
        id: connection

        property string title: qsTr("Подключение")
        readonly property string leftImagePath: "qrc:/images/controls/radio.svg"
        property bool isVisible: true
        readonly property var clickedHandler: function() {
            PageController.goToPage(PageEnum.PageSettingsConnection)
        }
    }

    QtObject {
        id: application

        property string title: qsTr("Приложение")
        readonly property string leftImagePath: "qrc:/images/controls/app.svg"
        property bool isVisible: true
        readonly property var clickedHandler: function() {
            PageController.goToPage(PageEnum.PageSettingsApplication)
        }
    }

    QtObject {
        id: news

        property string title: qsTr("Новости и уведомления")
        readonly property string leftImagePath: NewsModel.hasUnread && SettingsController.isNewsNotificationsEnabled() ? "qrc:/images/controls/news-unread.svg" : "qrc:/images/controls/news.svg"
        property bool isVisible: ServersModel.hasServersFromGatewayApi
        readonly property var clickedHandler: function() {
            if (!ServersModel.hasServersFromGatewayApi) {
                return;
            }
            PageController.showBusyIndicator(true)
            ApiNewsController.fetchNews(true)
            PageController.goToPage(PageEnum.PageSettingsNewsNotifications)
        }
    }



    QtObject {
        id: about

        property string title: qsTr("About FBLink VPN")
        readonly property string leftImagePath: "qrc:/images/controls/info.svg"
        property bool isVisible: true
        readonly property var clickedHandler: function() {
            PageController.goToPage(PageEnum.PageSettingsAbout)
        }
    }

    QtObject {
        id: devConsole

        property string title: qsTr("Консоль разработчика")
        readonly property string leftImagePath: "qrc:/images/controls/bug.svg"
        property bool isVisible: SettingsController.isDevModeEnabled
        readonly property var clickedHandler: function() {
            PageController.goToPage(PageEnum.PageDevMenu)
        }
    }
}
