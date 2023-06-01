import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    SortFilterProxyModel {
        id: proxyServersModel
        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

    ColumnLayout {
        id: content

        anchors.fill: parent

        spacing: 16

        Repeater {
            id: header
            model: proxyServersModel

            delegate: HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                actionButtonImage: "qrc:/images/controls/edit-3.svg"
                backButtonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: name
                descriptionText: hostName

                actionButtonFunction: function() {
                    connectionTypeSelection.visible = true
                }

                backButtonFunction: function() {
                    closePage()
                }
            }
        }

        TabBar {
            id: tabBar

            Layout.fillWidth: true

            background: Rectangle {
                color: "transparent"
            }

            TabButtonType {
                isSelected: tabBar.currentIndex === 0
                text: qsTr("Protocols")
//                onClicked: {
//                    tabBarStackView.goToTabBarPage(PageEnum.PageSettingsServerProtocols)
//                }
            }
            TabButtonType {
                isSelected: tabBar.currentIndex === 1
                text: qsTr("Services")
//                onClicked: {
//                    tabBarStackView.goToTabBarPage(PageEnum.PageSettingsServerServices)
//                }
            }
            TabButtonType {
                isSelected: tabBar.currentIndex === 2
                text: qsTr("Data")
//                onClicked: {
//                    tabBarStackView.goToTabBarPage(PageEnum.PageSettingsServerData)
//                }
            }
        }

        StackLayout {
            Layout.preferredWidth: root.width
            Layout.preferredHeight: root.height - tabBar.implicitHeight - header.implicitHeight

            currentIndex: tabBar.currentIndex

            PageSettingsServerProtocols {
                stackView: root.stackView
            }
            PageSettingsServerServices {
                stackView: root.stackView
            }
            PageSettingsServerData {
                stackView: root.stackView
            }
        }

//        StackViewType {
//            id: tabBarStackView

//            Layout.preferredWidth: root.width
//            Layout.preferredHeight: root.height - tabBar.implicitHeight - header.implicitHeight

//            function goToTabBarPage(page) {
//                var pagePath = PageController.getPagePath(page)
//                while (tabBarStackView.depth > 1) {
//                    tabBarStackView.pop()
//                }
//                tabBarStackView.replace(pagePath, { "objectName" : pagePath })
//            }

//            Component.onCompleted: {
//                var pagePath = PageController.getPagePath(PageEnum.PageSettingsServerProtocols)
//                tabBarStackView.push(pagePath, { "objectName" : pagePath })
//            }
//        }
    }
}
