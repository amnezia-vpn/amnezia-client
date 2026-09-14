import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    readonly property int pageSettingsServerProtocols: 0
    readonly property int pageSettingsServerServices: 1
    readonly property int pageSettingsServerData: 2

    property var processedServer

    Connections {
        target: PageController

        function onGoToPageSettingsServerServices() {
            tabBar.setCurrentIndex(root.pageSettingsServerServices)
        }
    }

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ServersModel

        function onModelReset() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "serverId"
                value: ServersUiController.processedServerId
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    ColumnLayout {
        objectName: "mainLayout"

        anchors.fill: parent
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        spacing: 4

        BackButtonType {
            id: backButton
            objectName: "backButton"
        }

        HeaderTypeWithButton {
            id: headerContent
            objectName: "headerContent"

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 10

            actionButtonImage: "qrc:/images/controls/edit-3.svg"

            headerText: root.processedServer != null ? root.processedServer.name : ""
            descriptionText: {
                if (root.processedServer == null) {
                    return ""
                }
                if (ServersUiController.isServerFromApi(ServersUiController.processedServerId)) {
                    return root.processedServer.serverDescription
                } else if (ServersUiController.isProcessedServerHasWriteAccess()) {
                    return root.processedServer.credentialsLogin + " · " + root.processedServer.hostName
                } else {
                    return root.processedServer.hostName
                }
            }

            actionButtonFunction: function() {
                serverNameEditDrawer.openTriggered()
            }
        }

        RenameServerDrawer {
            id: serverNameEditDrawer

            parent: root

            anchors.fill: parent
            expandedHeight: root.height * 0.35

            serverNameText: root.processedServer != null ? root.processedServer.name : ""
        }

        TabBar {
            id: tabBar

            Layout.fillWidth: true

            currentIndex: (ServersUiController.isServerFromApi(ServersUiController.processedServerId)
                           && !ServersUiController.serverHasInstalledContainers(ServersUiController.processedServerId)) ?
                              root.pageSettingsServerData : root.pageSettingsServerProtocols

            background: Rectangle {
                color: AmneziaStyle.color.transparent
            }


            TabButtonType {
                id: protocolsTab
                visible: protocolsPage.installedProtocolsCount
                width: protocolsPage.installedProtocolsCount ? undefined : 0
                isSelected: TabBar.tabBar.currentIndex === root.pageSettingsServerProtocols
                text: qsTr("Protocols")

                Keys.onReturnPressed: TabBar.tabBar.setCurrentIndex(root.pageSettingsServerProtocols)
                Keys.onEnterPressed: TabBar.tabBar.setCurrentIndex(root.pageSettingsServerProtocols)
            }

            TabButtonType {
                id: servicesTab
                visible: servicesPage.installedServicesCount
                width: servicesPage.installedServicesCount ? undefined : 0
                isSelected: TabBar.tabBar.currentIndex === root.pageSettingsServerServices
                text: qsTr("Services")

                Keys.onReturnPressed: TabBar.tabBar.setCurrentIndex(root.pageSettingsServerServices)
                Keys.onEnterPressed: TabBar.tabBar.setCurrentIndex(root.pageSettingsServerServices)
            }

            TabButtonType {
                id: dataTab
                isSelected: tabBar.currentIndex === root.pageSettingsServerData
                text: qsTr("Management")

                Keys.onReturnPressed: TabBar.tabBar.setCurrentIndex(root.pageSettingsServerData)
                Keys.onEnterPressed: TabBar.tabBar.setCurrentIndex(root.pageSettingsServerData)
            }
        }

        StackLayout {
            id: nestedStackView

            Layout.fillWidth: true

            currentIndex: tabBar.currentIndex

            PageSettingsServerProtocols {
                id: protocolsPage
                stackView: root.stackView
            }

            PageSettingsServerServices {
                id: servicesPage
                stackView: root.stackView
            }

            PageSettingsServerData {
                id: dataPage
                stackView: root.stackView
            }
        }
    }
}
