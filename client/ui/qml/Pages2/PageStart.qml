import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

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
            tabBarStackView.goToTabBarPage(PageController.getPagePath(PageEnum.PageHome))
        }

        function onGoToPageSettings() {
            tabBar.currentIndex = 2
            tabBarStackView.goToTabBarPage(PageController.getPagePath(PageEnum.PageSettings))
        }

        function onGoToPageViewConfig() {
            var pagePath = PageController.getPagePath(PageEnum.PageSetupWizardViewConfig)
            tabBarStackView.push(pagePath, { "objectName" : pagePath }, StackView.PushTransition)
        }

        function onShowErrorMessage(errorMessage) {
            popupErrorMessage.popupErrorMessageText = errorMessage
            popupErrorMessage.open()
        }

        function onShowBusyIndicator(visible) {
            busyIndicator.visible = visible
            tabBarStackView.enabled = !visible
            tabBar.enabled = !visible
        }

        function onClosePage() {
            if (tabBarStackView.depth <= 1) {
                return
            }
            tabBarStackView.pop()
        }
    }

    StackViewType {
        id: tabBarStackView

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: tabBar.top

        width: parent.width
        height: root.height - tabBar.implicitHeight

        function goToTabBarPage(page) {
            var pagePath = PageController.getPagePath(page)
            tabBarStackView.clear(StackView.PopTransition)
            tabBarStackView.replace(pagePath, { "objectName" : pagePath })
        }

        Component.onCompleted: {
            var pagePath = PageController.getPagePath(PageEnum.PageHome)
            ServersModel.currentlyProcessedIndex = ServersModel.defaultIndex
            tabBarStackView.push(pagePath, { "objectName" : pagePath })
        }
    }

    TabBar {
        id: tabBar

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        topPadding: 8
        bottomPadding: 8
        leftPadding: shareTabButton.visible ? 96 : 128
        rightPadding: shareTabButton.visible ? 96 : 128

        background: Shape {
            width: parent.width
            height: parent.height

            ShapePath {
                startX: 0
                startY: 0

                PathLine { x: width; y: 0 }
                PathLine { x: width; y: height - 1 }
                PathLine { x: 0; y: height - 1 }
                PathLine { x: 0; y: 0 }

                strokeWidth: 1
                strokeColor: "#2C2D30"
                fillColor: "#1C1D21"
            }
        }

        TabImageButtonType {
            isSelected: tabBar.currentIndex === 0
            image: "qrc:/images/controls/home.svg"
            onClicked: {
                tabBarStackView.goToTabBarPage(PageEnum.PageHome)
                ServersModel.currentlyProcessedIndex = ServersModel.defaultIndex
            }
        }
        TabImageButtonType {
            id: shareTabButton

            Connections {
                target: ServersModel

                function onDefaultServerIndexChanged() {
                    shareTabButton.visible = ServersModel.isCurrentlyProcessedServerHasWriteAccess()
                    shareTabButton.width = ServersModel.isCurrentlyProcessedServerHasWriteAccess() ? undefined : 0
                }
            }

            visible: ServersModel.isCurrentlyProcessedServerHasWriteAccess()
            width: ServersModel.isCurrentlyProcessedServerHasWriteAccess() ? undefined : 0

            isSelected: tabBar.currentIndex === 1
            image: "qrc:/images/controls/share-2.svg"
            onClicked: {
                tabBarStackView.goToTabBarPage(PageEnum.PageShare)
            }
        }
        TabImageButtonType {
            isSelected: tabBar.currentIndex === 2
            image: "qrc:/images/controls/settings-2.svg"
            onClicked: {
                tabBarStackView.goToTabBarPage(PageEnum.PageSettings)
            }
        }
    }

    MouseArea {
        anchors.fill: tabBar
        anchors.leftMargin: shareTabButton.visible ? 96 : 128
        anchors.rightMargin: shareTabButton.visible ? 96 : 128
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

    BusyIndicatorType {
        id: busyIndicator
        anchors.centerIn: parent
        z: 1
    }
}
