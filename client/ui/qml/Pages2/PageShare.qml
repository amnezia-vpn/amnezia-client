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

    enum ConfigType {
        AmneziaConnection,
        AmneziaFullAccess,
        OpenVpn,
        WireGuard
    }

    Connections {
        target: ExportController

        function onGenerateConfig(type) {
            shareConnectionDrawer.open()
            shareConnectionDrawer.contentVisible = false
            PageController.showBusyIndicator(true)

            switch (type) {
            case PageShare.ConfigType.AmneziaConnection: ExportController.generateConnectionConfig(); break;
            case PageShare.ConfigType.AmneziaFullAccess: {
                if (Qt.platform.os === "android") {
                    ExportController.generateFullAccessConfigAndroid();
                } else {
                    ExportController.generateFullAccessConfig();
                }
                break;
            }
            case PageShare.ConfigType.OpenVpn: ExportController.generateOpenVpnConfig(); break;
            case PageShare.ConfigType.WireGuard: ExportController.generateWireGuardConfig(); break;
            }

            PageController.showBusyIndicator(false)
            shareConnectionDrawer.contentVisible = true
        }

        function onExportErrorOccurred(errorMessage) {
            shareConnectionDrawer.close()
            PageController.showErrorMessage(errorMessage)
        }
    }

    property string fullConfigServerSelectorText
    property string connectionServerSelectorText
    property bool showContent: false
    property bool shareButtonEnabled: true
    property list<QtObject> connectionTypesModel: [
        amneziaConnectionFormat
    ]

    QtObject {
        id: amneziaConnectionFormat
        property string name: qsTr("For the AmneziaVPN app")
        property var type: PageShare.ConfigType.AmneziaConnection
    }
    QtObject {
        id: openVpnConnectionFormat
        property string name: qsTr("OpenVpn native format")
        property var type: PageShare.ConfigType.OpenVpn
    }
    QtObject {
        id: wireGuardConnectionFormat
        property string name: qsTr("WireGuard native format")
        property var type: PageShare.ConfigType.WireGuard
    }

    FlickableType {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
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
                            serverSelector.text = root.connectionServerSelectorText
                        }
                    }

                    HorizontalRadioButton {
                        checked: root.currentIndex === 1

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Full access")

                        onClicked: {
                            accessTypeSelector.currentIndex = 1
                            serverSelector.text = root.fullConfigServerSelectorText
                            root.shareButtonEnabled = true
                        }
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24

                text: accessTypeSelector.currentIndex === 0 ? qsTr("VPN access without the ability to manage the server") :
                                                              qsTr("Full access to server")
                color: "#878B91"
            }

            DropDownType {
                id: serverSelector

                Layout.fillWidth: true
                Layout.topMargin: 24

                implicitHeight: 74

                drawerHeight: 0.4375

                descriptionText: qsTr("Server and service")
                headerText: qsTr("Server")

                listView: ListViewType {
                    rootWidth: root.width
                    dividerVisible: true

                    imageSource: "qrc:/images/controls/chevron-right.svg"

                    model: SortFilterProxyModel {
                        id: proxyServersModel
                        sourceModel: ServersModel
                        filters: [
                            ValueFilter {
                                roleName: "hasWriteAccess"
                                value: true
                            }
                        ]
                    }

                    currentIndex: 0

                    clickedFunction: function() {
                        handler()

                        if (accessTypeSelector.currentIndex === 0) {
                            protocolSelector.visible = true
                            root.shareButtonEnabled = false
                        } else {
                            shareConnectionDrawer.headerText = qsTr("Accessing ") + serverSelector.text
                            shareConnectionDrawer.configContentHeaderText = qsTr("File with connection settings to ") + serverSelector.text
                            serverSelector.menuVisible = false
                        }
                    }

                    Component.onCompleted: {
                        handler()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        root.fullConfigServerSelectorText = selectedText
                        root.connectionServerSelectorText = selectedText
                        ServersModel.currentlyProcessedIndex = proxyServersModel.mapToSource(currentIndex)
                    }
                }

                DrawerType {
                    id: protocolSelector

                    width: parent.width
                    height: parent.height * 0.5

                    ColumnLayout {
                        id: protocolSelectorHeader

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        BackButtonType {
                            backButtonImage: "qrc:/images/controls/arrow-left.svg"
                            backButtonFunction: function() {
                                protocolSelector.visible = false
                            }
                        }
                    }

                    FlickableType {
                        anchors.top: protocolSelectorHeader.bottom
                        anchors.topMargin: 16
                        contentHeight: protocolSelectorContent.implicitHeight

                        Column {
                            id: protocolSelectorContent
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
                                    handler()

                                    protocolSelector.visible = false
                                    serverSelector.menuVisible = false
                                }

                                Component.onCompleted: {
                                    if (accessTypeSelector.currentIndex === 0) {
                                        handler()
                                    }
                                }

                                function handler() {
                                    if (!proxyContainersModel.count) {
                                        root.shareButtonEnabled = false
                                        return
                                    } else {
                                        root.shareButtonEnabled = true
                                    }

                                    serverSelector.text += ", " + selectedText
                                    root.connectionServerSelectorText = serverSelector.text

                                    shareConnectionDrawer.headerText = qsTr("Connection to ") + serverSelector.text
                                    shareConnectionDrawer.configContentHeaderText = qsTr("File with connection settings to ") + serverSelector.text
                                    ContainersModel.setCurrentlyProcessedContainerIndex(proxyContainersModel.mapToSource(currentIndex))

                                    fillConnectionTypeModel()
                                }

                                function fillConnectionTypeModel() {
                                    root.connectionTypesModel = [amneziaConnectionFormat]

                                    var index = proxyContainersModel.mapToSource(currentIndex)

                                    if (index === ContainerProps.containerFromString("amnezia-openvpn")) {
                                        root.connectionTypesModel.push(openVpnConnectionFormat)
                                    } else if (index === ContainerProps.containerFromString("amnezia-wireguard")) {
                                        root.connectionTypesModel.push(wireGuardConnectionFormat)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            DropDownType {
                id: exportTypeSelector

                property int currentIndex

                Layout.fillWidth: true
                Layout.topMargin: 16

                implicitHeight: 74

                drawerHeight: 0.4375

                visible: accessTypeSelector.currentIndex === 0
                enabled: root.connectionTypesModel.length > 1

                descriptionText: qsTr("Connection format")
                headerText: qsTr("Connection format")

                listView: ListViewType {
                    rootWidth: root.width
                    dividerVisible: true

                    imageSource: "qrc:/images/controls/chevron-right.svg"

                    model: root.connectionTypesModel
                    currentIndex: 0

                    clickedFunction: function() {
                        exportTypeSelector.text = selectedText
                        exportTypeSelector.currentIndex = currentIndex
                        exportTypeSelector.menuVisible = false
                    }

                    Component.onCompleted: {
                        exportTypeSelector.text = selectedText
                        exportTypeSelector.currentIndex = currentIndex
                    }
                }
            }

            ShareConnectionDrawer {
                id: shareConnectionDrawer
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 32

                enabled: shareButtonEnabled

                text: qsTr("Share")

                onClicked: {
                    if (accessTypeSelector.currentIndex === 0) {
                        ExportController.generateConfig(root.connectionTypesModel[exportTypeSelector.currentIndex].type)
                    } else {
                        ExportController.generateConfig(PageShare.ConfigType.AmneziaFullAccess)
                    }
                }
            }
        }
    }
}
