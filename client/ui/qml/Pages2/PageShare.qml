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
        OpenVpn,
        WireGuard,
        ShadowSocks,
        Cloak
    }

    signal revokeConfig(int index)
    onRevokeConfig: function(index) {
        PageController.showBusyIndicator(true)
        ExportController.revokeConfig(index,
                                      ContainersModel.getCurrentlyProcessedContainerIndex(),
                                      ServersModel.getCurrentlyProcessedServerCredentials())
        PageController.showBusyIndicator(false)
    }

    Connections {
        target: ExportController

        function onGenerateConfig(type) {
            shareConnectionDrawer.headerText = qsTr("Connection to ") + serverSelector.text
            shareConnectionDrawer.configContentHeaderText = qsTr("File with connection settings to ") + serverSelector.text

            shareConnectionDrawer.needCloseButton = false

            shareConnectionDrawer.open()
            shareConnectionDrawer.contentVisible = false
            PageController.showBusyIndicator(true)

            switch (type) {
            case PageShare.ConfigType.AmneziaConnection: ExportController.generateConnectionConfig(clientNameTextField.textFieldText); break;
            case PageShare.ConfigType.OpenVpn: {
                ExportController.generateOpenVpnConfig(clientNameTextField.textFieldText)
                shareConnectionDrawer.configCaption = qsTr("Save OpenVPN config")
                shareConnectionDrawer.configExtension = ".ovpn"
                shareConnectionDrawer.configFileName = "amnezia_for_openvpn"
                break
            }
            case PageShare.ConfigType.WireGuard: {
                ExportController.generateWireGuardConfig(clientNameTextField.textFieldText)
                shareConnectionDrawer.configCaption = qsTr("Save WireGuard config")
                shareConnectionDrawer.configExtension = ".conf"
                shareConnectionDrawer.configFileName = "amnezia_for_wireguard"
                break
            }
            case PageShare.ConfigType.ShadowSocks: {
                ExportController.generateShadowSocksConfig()
                shareConnectionDrawer.configCaption = qsTr("Save ShadowSocks config")
                shareConnectionDrawer.configExtension = ".json"
                shareConnectionDrawer.configFileName = "amnezia_for_shadowsocks"
                break
            }
            case PageShare.ConfigType.Cloak: {
                ExportController.generateCloakConfig()
                shareConnectionDrawer.configCaption = qsTr("Save Cloak config")
                shareConnectionDrawer.configExtension = ".json"
                shareConnectionDrawer.configFileName = "amnezia_for_cloak"
                break
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

    property bool isSearchBarVisible: false
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
    QtObject {
        id: shadowSocksConnectionFormat
        property string name: qsTr("ShadowSocks native format")
        property var type: PageShare.ConfigType.ShadowSocks
    }
    QtObject {
        id: cloakConnectionFormat
        property string name: qsTr("Cloak native format")
        property var type: PageShare.ConfigType.Cloak
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

                headerText: qsTr("Share VPN Access")

                actionButtonImage: "qrc:/images/controls/more-vertical.svg"
                actionButtonFunction: function() {
                    shareFullAccessDrawer.open()
                }

                DrawerType {
                    id: shareFullAccessDrawer

                    width: root.width
                    height: root.height * 0.45


                    ColumnLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        spacing: 0

                        Header2Type {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 16
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            headerText: qsTr("Share full access to the server and VPN")
                            descriptionText: qsTr("Use for your own devices, or share with those you trust to manage the server.")
                        }


                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: qsTr("Share")
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                PageController.goToPage(PageEnum.PageShareFullAccess)
                                shareFullAccessDrawer.close()
                            }
                        }
                    }
                }
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
                        checked: accessTypeSelector.currentIndex === 1

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Users")

                        onClicked: {
                            accessTypeSelector.currentIndex = 1
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ContainersModel.getCurrentlyProcessedContainerIndex(),
                                                                         ServersModel.getCurrentlyProcessedServerCredentials())
                            PageController.showBusyIndicator(false)
                        }
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24

                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share VPN access without the ability to manage the server")
                color: "#878B91"
            }

            TextFieldWithHeaderType {
                id: clientNameTextField
                Layout.fillWidth: true
                Layout.topMargin: 16

                visible: accessTypeSelector.currentIndex === 0

                headerText: qsTr("User name")
                textFieldText: "New client"

                checkEmptyText: true
            }

            DropDownType {
                id: serverSelector

                signal severSelectorIndexChanged
                property int currentIndex: -1

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

                    clickedFunction: function() {
                        handler()

                        if (serverSelector.currentIndex !== serverSelectorListView.currentIndex) {
                            serverSelector.currentIndex = serverSelectorListView.currentIndex
                            serverSelector.severSelectorIndexChanged()
                        }

                        serverSelector.menuVisible = false
                    }

                    Component.onCompleted: {
                        serverSelectorListView.currentIndex = ServersModel.isDefaultServerHasWriteAccess() ?
                                    proxyServersModel.mapFromSource(ServersModel.defaultIndex) : 0
                        serverSelectorListView.triggerCurrentItem()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        ServersModel.currentlyProcessedIndex = proxyServersModel.mapToSource(currentIndex)
                    }
                }
            }

            DropDownType {
                id: protocolSelector

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
                        protocolSelectorListView.currentItem.y

                        handler()

                        protocolSelector.menuVisible = false
                    }

                    Connections {
                        target: serverSelector

                        function onSeverSelectorIndexChanged() {
                            protocolSelectorListView.currentIndex = proxyContainersModel.mapFromSource(ContainersModel.getDefaultContainer())
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

                        ContainersModel.setCurrentlyProcessedContainerIndex(proxyContainersModel.mapToSource(currentIndex))

                        fillConnectionTypeModel()

                        if (accessTypeSelector.currentIndex === 1) {
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ContainersModel.getCurrentlyProcessedContainerIndex(),
                                                                         ServersModel.getCurrentlyProcessedServerCredentials())
                            PageController.showBusyIndicator(false)
                        }
                    }

                    function fillConnectionTypeModel() {
                        root.connectionTypesModel = [amneziaConnectionFormat]

                        var index = proxyContainersModel.mapToSource(currentIndex)

                        if (index === ContainerProps.containerFromString("amnezia-openvpn")) {
                            root.connectionTypesModel.push(openVpnConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-wireguard")) {
                            root.connectionTypesModel.push(wireGuardConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-shadowsocks")) {
                            root.connectionTypesModel.push(openVpnConnectionFormat)
                            root.connectionTypesModel.push(shadowSocksConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-openvpn-cloak")) {
                            root.connectionTypesModel.push(openVpnConnectionFormat)
                            root.connectionTypesModel.push(shadowSocksConnectionFormat)
                            root.connectionTypesModel.push(cloakConnectionFormat)
                        }
                    }
                }
            }

            DropDownType {
                id: exportTypeSelector

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
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 40

                enabled: shareButtonEnabled
                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share")
                imageSource: "qrc:/images/controls/share-2.svg"

                onClicked: {
                    ExportController.generateConfig(root.connectionTypesModel[exportTypeSelector.currentIndex].type)
                }
            }

            Header2Type {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 16

                visible: accessTypeSelector.currentIndex === 1 && !root.isSearchBarVisible

                headerText: qsTr("Users")
                actionButtonImage: "qrc:/images/controls/search.svg"
                actionButtonFunction: function() {
                    root.isSearchBarVisible = true
                }
            }

            RowLayout {
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                visible: accessTypeSelector.currentIndex === 1 && root.isSearchBarVisible

                TextFieldWithHeaderType {
                    id: searchTextField
                    Layout.fillWidth: true

                    textFieldPlaceholderText: qsTr("Search")
                }

                ImageButtonType {
                    image: "qrc:/images/controls/close.svg"
                    imageColor: "#D7D8DB"

                    onClicked: function() {
                        root.isSearchBarVisible = false
                        searchTextField.textFieldText = ""
                    }
                }
            }

            ListView {
                id: clientsListView
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height

                visible: accessTypeSelector.currentIndex === 1

                model: SortFilterProxyModel {
                    id: proxyClientManagementModel
                    sourceModel: ClientManagementModel
                    filters: RegExpFilter {
                        roleName: "clientName"
                        pattern: ".*" + searchTextField.textFieldText + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                }

                clip: true
                interactive: false

                delegate: Item {
                    implicitWidth: clientsListView.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        anchors.rightMargin: -16
                        anchors.leftMargin: -16

                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: clientName
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                clientInfoDrawer.open()
                            }
                        }

                        DividerType {}

                        DrawerType {
                            id: clientInfoDrawer

                            width: root.width
                            height: root.height * 0.5

                            ColumnLayout {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 16
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16

                                spacing: 8

                                Header2Type {
                                    Layout.fillWidth: true
                                    Layout.bottomMargin: 24

                                    headerText: clientName
                                    descriptionText: serverSelector.text
                                }

                                BasicButtonType {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 24

                                    defaultColor: "transparent"
                                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
                                    disabledColor: "#878B91"
                                    textColor: "#D7D8DB"
                                    borderWidth: 1

                                    text: qsTr("Rename")

                                    onClicked: function() {
                                        clientNameEditDrawer.open()
                                    }

                                    DrawerType {
                                        id: clientNameEditDrawer

                                        width: root.width
                                        height: root.height * 0.35

                                        onVisibleChanged: {
                                            if (clientNameEditDrawer.visible) {
                                                clientNameEditor.textField.forceActiveFocus()
                                            }
                                        }

                                        ColumnLayout {
                                            anchors.top: parent.top
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.topMargin: 16
                                            anchors.leftMargin: 16
                                            anchors.rightMargin: 16

                                            TextFieldWithHeaderType {
                                                id: clientNameEditor
                                                Layout.fillWidth: true
                                                headerText: qsTr("Client name")
                                                textFieldText: clientName
                                                textField.maximumLength: 30
                                            }

                                            BasicButtonType {
                                                Layout.fillWidth: true

                                                text: qsTr("Save")

                                                onClicked: {
                                                    if (clientNameEditor.textFieldText !== clientName) {
                                                        PageController.showBusyIndicator(true)
                                                        ExportController.renameClient(index,
                                                                                      clientNameEditor.textFieldText,
                                                                                      ContainersModel.getCurrentlyProcessedContainerIndex(),
                                                                                      ServersModel.getCurrentlyProcessedServerCredentials())
                                                        PageController.showBusyIndicator(false)
                                                        clientNameEditDrawer.close()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                BasicButtonType {
                                    Layout.fillWidth: true

                                    defaultColor: "transparent"
                                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
                                    disabledColor: "#878B91"
                                    textColor: "#D7D8DB"
                                    borderWidth: 1

                                    text: qsTr("Revoke")

                                    onClicked: function() {
                                        questionDrawer.headerText = qsTr("Revoke the config for a user - ") + clientName + "?"
                                        questionDrawer.descriptionText = qsTr("The user will no longer be able to connect to your server.")
                                        questionDrawer.yesButtonText = qsTr("Continue")
                                        questionDrawer.noButtonText = qsTr("Cancel")

                                        questionDrawer.yesButtonFunction = function() {
                                            questionDrawer.close()
                                            clientInfoDrawer.close()
                                            root.revokeConfig(index)
                                        }
                                        questionDrawer.noButtonFunction = function() {
                                            questionDrawer.close()
                                        }
                                        questionDrawer.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        onPressed: function(mouse) {
            forceActiveFocus()
            mouse.accepted = false
        }
    }
}
