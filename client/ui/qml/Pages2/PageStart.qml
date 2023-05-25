import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Connections {
        target: PageController

        function onGoToPageHome() {
            tabBar.currentIndex = 0
        }

        function onShowErrorMessage(errorMessage) {
            popupErrorMessage.popupErrorMessageText = errorMessage
            popupErrorMessage.open()
        }
    }

    StackLayout {
        id: stackLayout
        currentIndex: tabBar.currentIndex

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: tabBar.top

        width: parent.width
        height: root.height - tabBar.implicitHeight

        StackView {
            id: homeStackView
            initialItem: "PageHome.qml" //PageController.getPagePath(PageEnum.PageSettingsServersList)
        }

        Item {

        }

        StackView {
            id: settingsStackView
            initialItem: "PageSettingsServersList.qml" //PageController.getPagePath(PageEnum.PageSettingsServersList)
        }
    }

    TabBar {
        id: tabBar

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        topPadding: 8
        bottomPadding: 34
        leftPadding: 96
        rightPadding: 96

        background: Rectangle {
            color: "#1C1D21"
        }


        TabImageButtonType {
            isSelected: tabBar.currentIndex === 0
            image: "qrc:/images/controls/home.svg"
        }
        TabImageButtonType {
            isSelected: tabBar.currentIndex === 1
            image: "qrc:/images/controls/share-2.svg"
        }
        TabImageButtonType {
            isSelected: tabBar.currentIndex === 2
            image: "qrc:/images/controls/settings-2.svg"
        }
    }

    MouseArea {
        anchors.fill: tabBar
        anchors.leftMargin: 96
        anchors.rightMargin: 96
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }

    Item {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        implicitHeight: popupErrorMessage.height

        PopupType {
            id: popupErrorMessage
        }
    }
}
