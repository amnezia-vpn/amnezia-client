import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    Connections {
        target: ExportController

        function onGenerateConfig(isFullAccess) {
            if (isFullAccess) {
                ExportController.generateFullAccessConfig()
            } else {
                ExportController.generateConnectionConfig()
            }

            shareConnectionDrawer.configText = ExportController.getAmneziaCode()
            shareConnectionDrawer.qrCodes = ExportController.getQrCodes()
        }
    }

    property bool showContent: false
    property list<QtObject> connectionTypesModel: [
        amneziaConnectionFormat
    ]

    QtObject {
        id: amneziaConnectionFormat
        property string name: qsTr("For the AmnesiaVPN app")
        property var func: function() {
            ExportController.generateConfig(false)
        }
    }
    QtObject {
        id: openVpnConnectionFormat
        property string name: qsTr("OpenVpn native format")
        property var func: function() {
            console.log("Item 3 clicked")
        }
    }
    QtObject {
        id: wireGuardConnectionFormat
        property string name: qsTr("WireGuard native format")
        property var func: function() {
            console.log("Item 3 clicked")
        }
    }

    FlickableType {
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            anchors.rightMargin: 16
            anchors.leftMargin: 16

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20

                headerText: qsTr("VPN Access")
            }

            Rectangle {
                id: accessTypeSelector

                property int currentIndex

                Layout.topMargin: 32

                implicitWidth: accessTypeSelectorContent.implicitWidth
                implicitHeight: accessTypeSelectorContent.implicitHeight

                color: "#1C1D21"
                radius: 16

                RowLayout {
                    id: accessTypeSelectorContent

                    spacing: 0

                    HorizontalRadioButton {
                        checked: accessTypeSelector.currentIndex === 0

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Connection")

                        onClicked: {
                            accessTypeSelector.currentIndex = 0
                        }
                    }

                    HorizontalRadioButton {
                        checked: root.currentIndex === 1

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Full")

                        onClicked: {
                            accessTypeSelector.currentIndex = 1
                        }
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true

                text: qsTr("VPN access without the ability to manage the server")
                color: "#878B91"
            }

            DropDownType {
                id: serverSelector

                Layout.fillWidth: true
                Layout.topMargin: 24

                implicitHeight: 74

                rootButtonBorderWidth: 0
                drawerHeight: 0.4375

                descriptionText: qsTr("Server and service")
                headerText: qsTr("Server")

                listView: ListViewType {
                    rootWidth: root.width
                    dividerVisible: true

                    imageSource: "qrc:/images/controls/chevron-right.svg"

                    model: ServersModel
                    currentIndex: ServersModel.getDefaultServerIndex()

                    clickedFunction: function() {
                        serverSelector.text = selectedText
                        ContainersModel.setCurrentlyProcessedServerIndex(currentIndex)
                        protocolSelector.visible = true
                    }

                    Component.onCompleted: {
                        serverSelector.text = selectedText
                        ContainersModel.setCurrentlyProcessedServerIndex(currentIndex)
                    }
                }

                DrawerType {
                    id: protocolSelector

                    width: parent.width
                    height: parent.height * 0.5

                    ColumnLayout {
                        id: header

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        BackButtonType {
                            backButtonImage: "qrc:/images/controls/arrow-left.svg"
                            backButtonFunction: function() {
                                protocolSelector.visible = false
                            }
                        }
                    }

                    FlickableType {
                        anchors.top: header.bottom
                        anchors.topMargin: 16
                        contentHeight: col.implicitHeight

                        Column {
                            id: col
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right

                            spacing: 16

                            Header2TextType {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16

                                text: qsTr("Protocols and services")
                                wrapMode: Text.WordWrap
                            }

                            ListViewType {
                                rootWidth: root.width
                                dividerVisible: true

                                imageSource: "qrc:/images/controls/chevron-right.svg"

                                model: SortFilterProxyModel {
                                    id: proxyContainersModel
                                    sourceModel: ContainersModel
                                    filters: [
                                        ValueFilter {
                                            roleName: "isInstalled"
                                            value: true
                                        }

                                    ]
                                }

                                currentIndex: 0

                                clickedFunction: function () {
                                    serverSelector.text += ", " + selectedText
                                    shareConnectionDrawer.headerText = qsTr("Connection to ") + serverSelector.text

                                    protocolSelector.visible = false
                                    serverSelector.menuVisible = false

                                    fillConnectionTypeModel()
                                }

                                Component.onCompleted: {
                                    serverSelector.text += ", " + selectedText
                                    shareConnectionDrawer.headerText = qsTr("Connection to ") + serverSelector.text

                                    fillConnectionTypeModel()
                                }

                                function fillConnectionTypeModel() {
                                    connectionTypesModel = [amneziaConnectionFormat]

                                    if (currentIndex === ContainerProps.containerFromString("OpenVpn")) {
                                        connectionTypesModel.push(openVpnConnectionFormat)
                                    } else if (currentIndex === ContainerProps.containerFromString("wireGuardConnectionType")) {
                                        connectionTypesModel.push(amneziaConnectionFormat)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            DropDownType {
                id: connectionTypeSelector

                property int currentIndex

                Layout.fillWidth: true
                Layout.topMargin: 16

                implicitHeight: 74

                rootButtonBorderWidth: 0
                drawerHeight: 0.4375

                visible: accessTypeSelector.currentIndex === 0
                enabled: connectionTypesModel.length > 1

                descriptionText: qsTr("Connection format")
                headerText: qsTr("Connection format")

                listView: ListViewType {
                    id: connectionTypeSelectorListView

                    rootWidth: root.width
                    dividerVisible: true

                    imageSource: "qrc:/images/controls/chevron-right.svg"

                    model: connectionTypesModel
                    currentIndex: 0

                    clickedFunction: function() {
                        connectionTypeSelector.text = selectedText
                        connectionTypeSelector.currentIndex = currentIndex
                        connectionTypeSelector.menuVisible = false
                    }

                    Component.onCompleted: {
                        connectionTypeSelector.text = selectedText
                        connectionTypeSelector.currentIndex = currentIndex
                    }
                }
            }

            ShareConnectionDrawer {
                id: shareConnectionDrawer
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 32

                text: qsTr("Share")

                onClicked: {
                    if (accessTypeSelector.currentIndex === 0) {
                        connectionTypesModel[connectionTypeSelector.currentIndex].func()
                    } else {
                        ExportController.generateConfig(true)
                    }
                    shareConnectionDrawer.visible = true
                }
            }
        }
    }
}
