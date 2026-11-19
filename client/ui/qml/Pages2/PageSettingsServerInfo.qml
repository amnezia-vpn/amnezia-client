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

    readonly property var tabTitles: [qsTr("Protocols"), qsTr("Services"), qsTr("Management")]
    readonly property var availableTabs: {
        const tabs = []
        if (protocolsPage.installedProtocolsCount > 0) {
            tabs.push(root.pageSettingsServerProtocols)
        }
        if (servicesPage.installedServicesCount > 0) {
            tabs.push(root.pageSettingsServerServices)
        }
        tabs.push(root.pageSettingsServerData)
        return tabs
    }

    property int currentTab: (ServersUiController.isServerFromApi(ServersUiController.processedServerId)
                              && !ServersUiController.serverHasInstalledContainers(ServersUiController.processedServerId)) ?
                                 root.pageSettingsServerData : root.pageSettingsServerProtocols

    property var processedServer

    Connections {
        target: PageController

        function onGoToPageSettingsServerServices() {
            root.currentTab = root.pageSettingsServerServices
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

            currentIndex: root.availableTabs.indexOf(root.currentTab)

            Repeater {
                model: root.availableTabs

                TabButtonType {
                    required property int modelData

                    text: root.tabTitles[modelData]

                    onClicked: root.currentTab = modelData
                }
            }
        }

        StackLayout {
            id: nestedStackView

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 24

            currentIndex: root.currentTab

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
