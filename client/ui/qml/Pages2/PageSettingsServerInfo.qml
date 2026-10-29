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
        anchors.fill: parent

        spacing: 0

        NavigationBarType {
            id: headerContent

            Layout.fillWidth: true

            title: root.processedServer != null ? root.processedServer.name : ""
            description: {
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

            IconButtonType {
                id: editButton

                icon.source: "qrc:/images/controls/edit-3.svg"

                onClicked: serverNameEditDrawer.open()
            }
        }

        RenameServerDrawer {
            id: serverNameEditDrawer

            parent: root

            serverNameText: root.processedServer != null ? root.processedServer.name : ""
        }

        TabsType {
            id: tabBar

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            currentIndex: (ServersUiController.isServerFromApi(ServersUiController.processedServerId)
                           && !ServersUiController.serverHasInstalledContainers(ServersUiController.processedServerId)) ?
                              root.pageSettingsServerData : root.pageSettingsServerProtocols

            TabButtonType {
                id: protocolsTab
                visible: protocolsPage.installedProtocolsCount > 0
                text: qsTr("Protocols")
            }

            TabButtonType {
                id: servicesTab
                visible: servicesPage.installedServicesCount > 0
                text: qsTr("Services")
            }

            TabButtonType {
                id: dataTab
                text: qsTr("Management")
            }
        }

        StackLayout {
            id: nestedStackView

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 24

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
