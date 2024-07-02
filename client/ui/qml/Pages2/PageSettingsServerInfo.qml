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

    defaultActiveFocusItem: focusItem

    Connections {
        target: PageController

        function onGoToPageSettingsServerServices() {
            tabBar.currentIndex = 1
        }
    }

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

    Item {
        id: focusItem
        KeyNavigation.tab: header
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 16

        Repeater {
            id: header
            model: proxyServersModel

            activeFocusOnTab: true
            onFocusChanged: {
                header.itemAt(0).focusItem.forceActiveFocus()
            }

            delegate: ColumnLayout {

                property alias focusItem: backButton

                id: content

                Layout.topMargin: 20

                BackButtonType {
                    id: backButton
                    KeyNavigation.tab: headerContent.actionButton
                }

                HeaderType {
                    id: headerContent
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    actionButtonImage: "qrc:/images/controls/edit-3.svg"

                    headerText: name
                    descriptionText: {
                        if (ServersModel.isProcessedServerHasWriteAccess()) {
                            return credentialsLogin + " · " + hostName
                        } else {
                            return hostName
                        }
                    }

                    KeyNavigation.tab: tabBar

                    actionButtonFunction: function() {
                        serverNameEditDrawer.open()
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
                            textFieldText: name
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

                                if (serverName.textFieldText !== name) {
                                    name = serverName.textFieldText
                                }
                                serverNameEditDrawer.close()
                            }
                        }
                    }
                }
            }
        }

        TabBar {
            id: tabBar

            Layout.fillWidth: true

            currentIndex: (ServersModel.getProcessedServerData("isServerFromTelegramApi")
                           && !ServersModel.getProcessedServerData("hasInstalledContainers")) ? 2 : 0

            background: Rectangle {
                color: "transparent"
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
                isSelected: tabBar.currentIndex === 0
                text: qsTr("Protocols")

                KeyNavigation.tab: servicesTab
                Keys.onReturnPressed: tabBar.currentIndex = 0
                Keys.onEnterPressed: tabBar.currentIndex = 0
            }

            TabButtonType {
                id: servicesTab
                visible: servicesPage.installedServicesCount
                width: servicesPage.installedServicesCount ? undefined : 0
                isSelected: tabBar.currentIndex === 1
                text: qsTr("Services")

                KeyNavigation.tab: dataTab
                Keys.onReturnPressed: tabBar.currentIndex = 1
                Keys.onEnterPressed: tabBar.currentIndex = 1
            }

            TabButtonType {
                id: dataTab
                isSelected: tabBar.currentIndex === 2
                text: qsTr("Management")

                Keys.onReturnPressed: tabBar.currentIndex = 2
                Keys.onEnterPressed: tabBar.currentIndex = 2
                KeyNavigation.tab: stackView.currentIndex === 0 ?
                                       protocolsPage :
                                       stackView.currentIndex === 1 ?
                                           servicesPage :
                                           dataPage
            }
        }

        StackLayout {
            id: stackView
            Layout.preferredWidth: root.width
            Layout.preferredHeight: root.height - tabBar.implicitHeight - header.implicitHeight

            currentIndex: ServersModel.getProcessedServerData("isServerFromGatewayApi") ? 3 : tabBar.currentIndex

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
        }

    }
}
