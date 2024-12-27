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

    property int pageSettingsServerProtocols: 0
    property int pageSettingsServerServices: 1
    property int pageSettingsServerData: 2
    property int pageSettingsApiServerInfo: 3
    property int pageSettingsApiLanguageList: 4

    property var processedServer

    defaultActiveFocusItem: focusItem

    Connections {
        target: PageController

        function onGoToPageSettingsServerServices() {
            tabBar.currentIndex = root.pageSettingsServerServices
        }
    }

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
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
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Item {
        id: focusItem
        //KeyNavigation.tab: header
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 4

        BackButtonType {
            id: backButton

            Layout.topMargin: 20
            KeyNavigation.tab: headerContent.actionButton

            backButtonFunction: function() {
                if (nestedStackView.currentIndex === root.pageSettingsApiServerInfo &&
                        root.processedServer.isCountrySelectionAvailable) {
                    nestedStackView.currentIndex = root.pageSettingsApiLanguageList
                } else {
                    PageController.closePage()
                }
            }
        }

        HeaderType {
            id: headerContent
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            actionButtonImage: nestedStackView.currentIndex === root.pageSettingsApiLanguageList ? "qrc:/images/controls/settings.svg"
                                                                                                 : "qrc:/images/controls/edit-3.svg"

            headerText: root.processedServer.name
            descriptionText: {
                if (root.processedServer.isServerFromGatewayApi) {
                    if (nestedStackView.currentIndex === root.pageSettingsApiLanguageList) {
                        return qsTr("Subscription is valid until ") + ApiServicesModel.getSelectedServiceData("endDate")
                    } else {
                        return ApiServicesModel.getSelectedServiceData("serviceDescription")
                    }
                } else if (root.processedServer.isServerFromTelegramApi) {
                    return root.processedServer.serverDescription
                } else if (root.processedServer.hasWriteAccess) {
                    return root.processedServer.credentialsLogin + " · " + root.processedServer.hostName
                } else {
                    return root.processedServer.hostName
                }
            }

            KeyNavigation.tab: tabBar

            actionButtonFunction: function() {
                if (nestedStackView.currentIndex === root.pageSettingsApiLanguageList) {
                    nestedStackView.currentIndex = root.pageSettingsApiServerInfo
                } else {
                    serverNameEditDrawer.open()
                }
            }
        }

        DrawerType2 {
            id: serverNameEditDrawer

            parent: root

            anchors.fill: parent
            expandedHeight: root.height * 0.35

            onClosed: {
                if (!GC.isMobile()) {
                    headerContent.actionButton.forceActiveFocus()
                }
            }

            expandedContent: ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 32
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Connections {
                    target: serverNameEditDrawer
                    enabled: !GC.isMobile()
                    function onOpened() {
                        serverName.textField.forceActiveFocus()
                    }
                }

                Item {
                    id: focusItem1
                    KeyNavigation.tab: serverName.textField
                }

                TextFieldWithHeaderType {
                    id: serverName

                    Layout.fillWidth: true
                    headerText: qsTr("Server name")
                    textFieldText: root.processedServer.name
                    textField.maximumLength: 30
                    checkEmptyText: true

                    KeyNavigation.tab: saveButton
                }

                BasicButtonType {
                    id: saveButton

                    Layout.fillWidth: true

                    text: qsTr("Save")
                    KeyNavigation.tab: focusItem1

                    clickedFunc: function() {
                        if (serverName.textFieldText === "") {
                            return
                        }

                        if (serverName.textFieldText !== root.processedServer.name) {
                            ServersModel.setProcessedServerData("name", serverName.textFieldText);
                        }
                        serverNameEditDrawer.close()
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

            activeFocusOnTab: true
            onFocusChanged: {
                if (activeFocus) {
                    protocolsTab.forceActiveFocus()
                }
            }

            TabButtonType {
                id: protocolsTab
                visible: protocolsPage.installedProtocolsCount
                width: protocolsPage.installedProtocolsCount ? undefined : 0
                isSelected: tabBar.currentIndex === root.pageSettingsServerProtocols
                text: qsTr("Protocols")

                KeyNavigation.tab: servicesTab
                Keys.onReturnPressed: tabBar.currentIndex = root.pageSettingsServerProtocols
                Keys.onEnterPressed: tabBar.currentIndex = root.pageSettingsServerProtocols
            }

            TabButtonType {
                id: servicesTab
                visible: servicesPage.installedServicesCount
                width: servicesPage.installedServicesCount ? undefined : 0
                isSelected: tabBar.currentIndex === root.pageSettingsServerServices
                text: qsTr("Services")

                KeyNavigation.tab: dataTab
                Keys.onReturnPressed: tabBar.currentIndex = root.pageSettingsServerServices
                Keys.onEnterPressed: tabBar.currentIndex = root.pageSettingsServerServices
            }

            TabButtonType {
                id: dataTab
                isSelected: tabBar.currentIndex === root.pageSettingsServerData
                text: qsTr("Management")

                Keys.onReturnPressed: tabBar.currentIndex = root.pageSettingsServerData
                Keys.onEnterPressed: tabBar.currentIndex = root.pageSettingsServerData
                Keys.onTabPressed: function() {
                    if (nestedStackView.currentIndex === root.pageSettingsServerProtocols) {
                        return protocolsPage
                    } else if (nestedStackView.currentIndex === root.pageSettingsServerProtocols) {
                        return servicesPage
                    } else {
                        return dataPage
                    }
                }
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

                onLastItemTabClickedSignal: lastItemTabClicked(focusItem)
            }

            PageSettingsServerServices {
                id: servicesPage
                stackView: root.stackView

                onLastItemTabClickedSignal: lastItemTabClicked(focusItem)
            }

            PageSettingsServerData {
                id: dataPage
                stackView: root.stackView

                onLastItemTabClickedSignal: lastItemTabClicked(focusItem)
            }

            PageSettingsApiServerInfo {
                id: apiInfoPage
                stackView: root.stackView

//                onLastItemTabClickedSignal: lastItemTabClicked(focusItem)
            }

            PageSettingsApiLanguageList {
                id: apiLanguageListPage
                stackView: root.stackView

//                onLastItemTabClickedSignal: lastItemTabClicked(focusItem)
            }
        }

    }
}
