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

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true
                Layout.bottomMargin: 16

                toolbarVisible: false
                backButtonVisible: false
                title: qsTr("Settings")
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                leadingSlotWidth: 24
                text: qsTr("Servers")
                icon.source: "qrc:/images/controls/server.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageSettingsServersList)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                leadingSlotWidth: 24
                text: qsTr("Connection")
                icon.source: "qrc:/images/controls/radio.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageSettingsConnection)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                leadingSlotWidth: 24
                text: qsTr("Application")
                icon.source: "qrc:/images/controls/app.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageSettingsApplication)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersUiController.hasServersFromGatewayApi

                leadingSlotWidth: 24
                text: qsTr("News & Notifications")
                icon.source: "qrc:/images/controls/news.svg"
                showIconBadge: NewsModel.hasUnread && SettingsController.isNewsNotificationsEnabled()
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    PageController.showBusyIndicator(true)
                    ApiNewsController.fetchNews(true)
                    PageController.goToPage(PageEnum.PageSettingsNewsNotifications)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                leadingSlotWidth: 24
                text: qsTr("Backup")
                icon.source: "qrc:/images/controls/save.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageSettingsBackup)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                leadingSlotWidth: 24
                text: qsTr("About AmneziaVPN")
                icon.source: "qrc:/images/controls/amnezia.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageSettingsAbout)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: SettingsController.isDevModeEnabled

                leadingSlotWidth: 24
                text: qsTr("Dev console")
                icon.source: "qrc:/images/controls/bug.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageDevMenu)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: GC.isDesktop()

                leadingSlotWidth: 24
                text: qsTr("Close application")
                icon.source: "qrc:/images/controls/x-circle.svg"
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.closeApplication()
            }
        }
    }
}
