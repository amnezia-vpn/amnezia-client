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

    ColumnLayout {
        anchors.fill: parent

        spacing: 16

        Repeater {
            id: header
            model: proxyServersModel

            delegate: ColumnLayout {
                id: content

                Layout.topMargin: 20

                BackButtonType {
                }

                HeaderType {
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

                    actionButtonFunction: function() {
                        serverNameEditDrawer.open()
                    }
                }

                DrawerType2 {
                    id: serverNameEditDrawer

                    parent: root

                    anchors.fill: parent
                    expandedHeight: root.height * 0.35

                    expandedContent: ColumnLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 32
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        Connections {
                            target: serverNameEditDrawer
                            function onOpened() {
                                serverName.textField.forceActiveFocus()
                            }
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

                        Component.onCompleted: {
                            if (header.itemAt(0)) {
                                defaultActiveFocusItem = serverName.textField
                            }
                        }
                    }
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
                visible: protocolsPage.installedProtocolsCount
                width: protocolsPage.installedProtocolsCount ? undefined : 0
                isSelected: tabBar.currentIndex === 0
                text: qsTr("Protocols")
            }
            TabButtonType {
                visible: servicesPage.installedServicesCount
                width: servicesPage.installedServicesCount ? undefined : 0
                isSelected: tabBar.currentIndex === 1
                text: qsTr("Services")
            }
            TabButtonType {
                isSelected: tabBar.currentIndex === 2
                text: qsTr("Data")
            }
        }

        StackLayout {
            Layout.preferredWidth: root.width
            Layout.preferredHeight: root.height - tabBar.implicitHeight - header.implicitHeight

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
                stackView: root.stackView
            }
        }
    }
}
