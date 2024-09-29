import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0
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
    readonly property int pageSettingsApiServerInfo: 3
    readonly property int pageSettingsApiLanguageList: 4

    property var server

    Connections {
        target: PageController

        function onGoToPageSettingsServerServices() {
            tabBar.setCurrentIndex(root.pageSettingsServerServices)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"
        
        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]

        Component.onCompleted: {
            root.server = proxyServersModel.get(0)
        }
    }

    ColumnLayout {
        objectName: "mainLayout"

        anchors.fill: parent
        anchors.topMargin: 20

        spacing: 4

        BackButtonType {
            id: backButton
            objectName: "backButton"

            backButtonFunction: function() {
                if (nestedStackView.currentIndex === root.pageSettingsApiServerInfo
                        && ServersModel.getProcessedServerData("isCountrySelectionAvailable")) {
                    nestedStackView.currentIndex = root.pageSettingsApiLanguageList
                } else {
                    PageController.closePage()
                }
            }
        }

        HeaderType {
            id: headerContent
            objectName: "headerContent"

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 10

            actionButtonImage: nestedStackView.currentIndex === root.pageSettingsApiLanguageList ? "qrc:/images/controls/settings.svg" : "qrc:/images/controls/edit-3.svg"

            headerText: root.server.name
            descriptionText: {
                if (ServersModel.getProcessedServerData("isServerFromGatewayApi")) {
                    return ApiServicesModel.getSelectedServiceData("serviceDescription")
                } else if (ServersModel.getProcessedServerData("isServerFromTelegramApi")) {
                    return root.server.serverDescription
                } else if (ServersModel.isProcessedServerHasWriteAccess()) {
                    return root.server.credentialsLogin + " · " + root.server.hostName
                } else {
                    return root.server.hostName
                }
            }

            actionButtonFunction: function() {
                if (nestedStackView.currentIndex === root.pageSettingsApiLanguageList) {
                    nestedStackView.currentIndex = root.pageSettingsApiServerInfo
                } else {
                    serverNameEditDrawer.openTriggered()
                }
            }
        }

        DrawerType2 {
            id: serverNameEditDrawer
            objectName: "serverNameEditDrawer"

            parent: root

            anchors.fill: parent
            expandedHeight: root.height * 0.35

            expandedStateContent: ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 32
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                TextFieldWithHeaderType {
                    id: serverName

                    Layout.fillWidth: true
                    headerText: qsTr("Server name")
                    textFieldText: root.server.name
                    textField.maximumLength: 30
                    checkEmptyText: true
                }

                BasicButtonType {
                    id: saveButton

                    Layout.fillWidth: true

                    text: qsTr("Save")

                    clickedFunc: function() {
                        if (serverName.textFieldText === "") {
                            return
                        }

                        if (serverName.textFieldText !== root.server.name) {
                            root.server.name = serverName.textFieldText // TODO(CyAn84): set value to the model
                        }
                        serverNameEditDrawer.closeTriggered()
                    }
                }
            }
        }

        TabBar {
            id: tabBar

            Layout.fillWidth: true

            currentIndex: (ServersModel.getProcessedServerData("isServerFromTelegramApi")
                           && !ServersModel.getProcessedServerData("hasInstalledContainers")) ?
                              root.pageSettingsServerData : root.pageSettingsServerProtocols

            background: Rectangle {
                color: AmneziaStyle.color.transparent
            }

            visible: !ServersModel.getProcessedServerData("isServerFromGatewayApi")


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

            currentIndex: ServersModel.getProcessedServerData("isServerFromGatewayApi") ?
                              (ServersModel.getProcessedServerData("isCountrySelectionAvailable") ?
                                   root.pageSettingsApiLanguageList : root.pageSettingsApiServerInfo) : tabBar.currentIndex

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

            PageSettingsApiServerInfo {
                id: apiInfoPage
                stackView: root.stackView
            }

            PageSettingsApiLanguageList {
                id: apiLanguageListPage
                stackView: root.stackView
            }
        }

    }
}
