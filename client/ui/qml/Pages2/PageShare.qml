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
            shareConnectionDrawer.needCloseButton = false

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
            case PageShare.ConfigType.OpenVpn: {
                ExportController.generateOpenVpnConfig();
                shareConnectionDrawer.configCaption = qsTr("Save OpenVPN config")
                shareConnectionDrawer.configExtension = ".ovpn"
                shareConnectionDrawer.configFileName = "amnezia_for_openvpn"
                break;
            }
            case PageShare.ConfigType.WireGuard: {
                ExportController.generateWireGuardConfig();
                shareConnectionDrawer.configCaption = qsTr("Save WireGuard config")
                shareConnectionDrawer.configExtension = ".conf"
                shareConnectionDrawer.configFileName = "amnezia_for_wireguard"
                break;
            }
            }

            PageController.showBusyIndicator(false)

            shareConnectionDrawer.needCloseButton = true
            PageController.showTopCloseButton(true)

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

            spacing: 0

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 24

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
                Layout.bottomMargin: 24

                text: accessTypeSelector.currentIndex === 0 ? qsTr("VPN access without the ability to manage the server") :
                                                              qsTr("Access to server management. The user with whom you share full access to the connection will be able to add and remove your protocols and services to the servers, as well as change settings.")
                color: "#878B91"
            }

            DropDownType {
                id: serverSelector

                drawerParent: root

                signal severSelectorIndexChanged
                property int currentIndex: 0

                Layout.fillWidth: true
                Layout.topMargin: 16

                drawerHeight: 0.4375

                descriptionText: qsTr("Server")
                headerText: qsTr("Server")

                listView: ListViewWithRadioButtonType {
                    id: serverSelectorListView

                    rootWidth: root.width
                    imageSource: "qrc:/images/controls/check.svg"

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

                        if (serverSelector.currentIndex !== serverSelectorListView.currentIndex) {
                            serverSelector.currentIndex = serverSelectorListView.currentIndex
                            serverSelector.severSelectorIndexChanged()
                        }

                        if (accessTypeSelector.currentIndex !== 0) {
                            shareConnectionDrawer.headerText = qsTr("Accessing ") + serverSelector.text
                            shareConnectionDrawer.configContentHeaderText = qsTr("File with accessing settings to ") + serverSelector.text
                        }
                        serverSelector.menuVisible = false
                    }

                    Component.onCompleted: {
                        handler()
                        serverSelector.severSelectorIndexChanged()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        root.fullConfigServerSelectorText = selectedText
                        root.connectionServerSelectorText = selectedText
                        ServersModel.currentlyProcessedIndex = proxyServersModel.mapToSource(currentIndex)
                    }
                }
            }

            DropDownType {
                id: protocolSelector

                drawerParent: root

                visible: accessTypeSelector.currentIndex === 0

                Layout.fillWidth: true
                Layout.topMargin: 16

                drawerHeight: 0.5

                descriptionText: qsTr("Protocol")
                headerText: qsTr("Protocol")

                listView: ListViewWithRadioButtonType {
                    id: protocolSelectorListView

                    rootWidth: root.width
                    imageSource: "qrc:/images/controls/check.svg"

                    model: SortFilterProxyModel {
                        id: proxyContainersModel
                        sourceModel: ContainersModel
                        filters: [
                            ValueFilter {
                                roleName: "isInstalled"
                                value: true
                            },
                            ValueFilter {
                                roleName: "isShareable"
                                value: true
                            }
                        ]
                    }

                    currentIndex: 0

                    clickedFunction: function() {
                        handler()

                        protocolSelector.menuVisible = false
                    }

                    Component.onCompleted: {
                        if (accessTypeSelector.currentIndex === 0) {
                            handler()
                        }
                    }

                    Connections {
                        target: serverSelector

                        function onSeverSelectorIndexChanged() {
                            protocolSelectorListView.currentIndex = 0
                            protocolSelectorListView.triggerCurrentItem()
                        }
                    }

                    function handler() {
                        if (!proxyContainersModel.count) {
                            root.shareButtonEnabled = false
                            return
                        } else {
                            root.shareButtonEnabled = true
                        }

                        protocolSelector.text = selectedText
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

            DropDownType {
                id: exportTypeSelector

                drawerParent: root

                property int currentIndex: 0

                Layout.fillWidth: true
                Layout.topMargin: 16

                drawerHeight: 0.4375

                visible: accessTypeSelector.currentIndex === 0
                enabled: root.connectionTypesModel.length > 1

                descriptionText: qsTr("Connection format")
                headerText: qsTr("Connection format")

                listView: ListViewWithRadioButtonType {
                    onCurrentIndexChanged: {
                        exportTypeSelector.currentIndex = currentIndex
                        exportTypeSelector.text = selectedText
                    }

                    rootWidth: root.width

                    imageSource: "qrc:/images/controls/check.svg"

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
                parent: root
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 40

                enabled: shareButtonEnabled

                text: qsTr("Share")
                imageSource: "qrc:/images/controls/share-2.svg"

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
