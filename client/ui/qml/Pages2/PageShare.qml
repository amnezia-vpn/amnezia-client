import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"


PageType {
    id: root

    enum ConfigType {
        AmneziaConnection,
        OpenVpn,
        WireGuard,
        Awg,
        Xray
    }

    Connections {
        target: ExportController

        function onRevokeConfigFinished() {
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(qsTr("Config revoked"))
        }

        function onGenerateConfig(type) {
            PageController.showBusyIndicator(true)

            var configCaption
            var configExtension
            var configFileName

            var containerIndex = ServersUiController.processedContainerIndex
            var serverId = ServersUiController.processedServerId

            switch (type) {
            case PageShare.ConfigType.AmneziaConnection: {
                ExportController.generateConnectionConfig(serverId, containerIndex, clientNameTextField.textField.text);
                configCaption = qsTr("Save AmneziaVPN config")
                configExtension = ".vpn"
                configFileName = "amnezia_config"
                break;
            }
            case PageShare.ConfigType.OpenVpn: {
                ExportController.generateOpenVpnConfig(serverId, clientNameTextField.textField.text)
                configCaption = qsTr("Save OpenVPN config")
                configExtension = ".ovpn"
                configFileName = "amnezia_for_openvpn"
                break
            }
            case PageShare.ConfigType.WireGuard: {
                ExportController.generateWireGuardConfig(serverId, clientNameTextField.textField.text)
                configCaption = qsTr("Save WireGuard config")
                configExtension = ".conf"
                configFileName = "amnezia_for_wireguard"
                break
            }
            case PageShare.ConfigType.Awg: {
                ExportController.generateAwgConfig(serverId, containerIndex, clientNameTextField.textField.text)
                configCaption = qsTr("Save AmneziaWG config")
                configExtension = ".conf"
                configFileName = "amnezia_for_awg"
                break
            }
            case PageShare.ConfigType.Xray: {
                ExportController.generateXrayConfig(serverId, clientNameTextField.textField.text)
                configCaption = qsTr("Save XRay config")
                configExtension = ".json"
                configFileName = "amnezia_for_xray"
                break
            }
            }

            PageController.showBusyIndicator(false)
            
            var headerText = qsTr("Connection to ") + serverSelector.text
            var configContentHeaderText = qsTr("File with connection settings to ") + serverSelector.text
            PageController.goToShareConnectionPage(headerText, configContentHeaderText, configCaption, configExtension, configFileName)
        }

        function onExportErrorOccurred(error) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(error)
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
        readonly property string name: qsTr("For the AmneziaVPN app")
        readonly property int type: PageShare.ConfigType.AmneziaConnection
    }
    QtObject {
        id: openVpnConnectionFormat
        readonly property string name: qsTr("OpenVPN native format")
        readonly property int type: PageShare.ConfigType.OpenVpn
    }
    QtObject {
        id: wireGuardConnectionFormat
        readonly property string name: qsTr("WireGuard native format")
        readonly property int type: PageShare.ConfigType.WireGuard
    }
    QtObject {
        id: awgConnectionFormat
        readonly property string name: qsTr("AmneziaWG native format")
        readonly property int type: PageShare.ConfigType.Awg
    }
    QtObject {
        id: xrayConnectionFormat
        readonly property string name: qsTr("XRay native format")
        readonly property int type: PageShare.ConfigType.Xray
    }

    ShareClientInfoSheet {
        id: clientInfoDrawer

        property int clientIndex: -1
        property string clientId

        parent: root

        function show(index, data) {
            clientInfoDrawer.clientIndex = index
            clientInfoDrawer.clientId = data.clientId || ""
            clientInfoDrawer.clientName = data.clientName || ""
            clientInfoDrawer.creationDate = data.creationDate || ""
            clientInfoDrawer.latestHandshake = data.latestHandshake || ""
            clientInfoDrawer.dataReceived = data.dataReceived || ""
            clientInfoDrawer.dataSent = data.dataSent || ""
            clientInfoDrawer.allowedIps = data.allowedIps || ""
            clientInfoDrawer.open()
        }

        onRenameRequested: function(newName) {
            clientsListView.freezeFilter = true
            PageController.showBusyIndicator(true)
            ExportController.renameClient(proxyClientManagementModel.mapToSource(clientInfoDrawer.clientIndex),
                                          newName,
                                          ServersUiController.processedServerId,
                                          ServersUiController.processedContainerIndex)
            PageController.showBusyIndicator(false)
            Qt.callLater(function(){ clientsListView.freezeFilter = false })
            clientInfoDrawer.clientName = newName
        }

        onRevokeRequested: {
            var headerText = qsTr("Revoke the config for a user - %1?").arg(clientInfoDrawer.clientName)
            var descriptionText = qsTr("The user will no longer be able to connect to your server.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                clientInfoDrawer.close()
                PageController.showBusyIndicator(true)
                ExportController.revokeConfig(proxyClientManagementModel.mapToSource(clientInfoDrawer.clientIndex),
                                              ServersUiController.processedServerId,
                                              ServersUiController.processedContainerIndex)
            }
            var noButtonFunction = function() {
            }

            if (ConnectionController.isRevokeBlockedDuringActiveConnection(
                    ServersUiController.processedServerId,
                    ServersUiController.processedContainerIndex,
                    clientInfoDrawer.clientId)) {
                PageController.showNotificationMessage("Unable to revoke current config during active connection")
            } else {
                showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
            }
        }
    }

    SheetType {
        id: shareFullAccessDrawer

        parent: root

        expandedHeight: shareFullAccessDrawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: shareFullAccessDrawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16

            spacing: 0

            SectionHeaderType {
                Layout.fillWidth: true
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Share full access to the server and VPN")
                descriptionText: qsTr("Use for your own devices, or share with those you trust to manage the server.")
            }

            ListItemType {
                id: shareFullAccessButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Share")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    PageController.goToPage(PageEnum.PageShareFullAccess)
                    shareFullAccessDrawer.close()
                }
            }
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: parent.bottom

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                id: header

                Layout.fillWidth: true

                backButtonVisible: false
                title: qsTr("Share VPN Access")

                IconButtonType {
                    size: IconButtonType.Small
                    icon.source: "qrc:/images/controls/more-vertical.svg"

                    onClicked: shareFullAccessDrawer.open()
                }
            }

            SegmentedControlType {
                id: accessTypeSelector

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                model: [qsTr("Connection"), qsTr("Users")]

                onActivated: function(index) {
                    if (index === 1) {
                        PageController.showBusyIndicator(true)
                        ExportController.updateClientManagementModel(ServersUiController.processedServerId,
                                                                     ServersUiController.processedContainerIndex)
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share VPN access without the ability to manage the server")
                color: AmneziaStyle.color.textTertiary
            }

            TextFieldType {
                id: clientNameTextField

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: accessTypeSelector.currentIndex === 0

                headerText: qsTr("User name")
                textField.text: "New client"
                textField.maximumLength: 20

                checkEmptyText: true
            }

            DropDownType {
                id: serverSelector

                signal serverSelectorIndexChanged
                property int currentIndex: -1

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                drawerHeight: 0.4375

                descriptionText: qsTr("Server")
                headerText: qsTr("Server")

                listView: RadioListType {
                    id: serverSelectorListView

                    model: SortFilterProxyModel {
                        id: proxyServersModel
                        sourceModel: ServersModel
                        filters: [
                            ValueFilter {
                                roleName: "hasWriteAccess"
                                value: true
                            },
                            ValueFilter {
                                roleName: "hasInstalledContainers"
                                value: true
                            }
                        ]
                    }

                    onActivated: {
                        handler()

                        if (serverSelector.currentIndex !== serverSelectorListView.selectedIndex) {
                            serverSelector.currentIndex = serverSelectorListView.selectedIndex
                            serverSelector.serverSelectorIndexChanged()
                        }

                        serverSelector.close()
                    }

                    Component.onCompleted: {
                        if (ServersUiController.isServerHasWriteAccess(ServersUiController.defaultServerId)
                            && ServersUiController.serverHasInstalledContainers(ServersUiController.defaultServerId)) {
                            serverSelectorListView.selectedIndex =
                                proxyServersModel.mapFromSource(ServersUiController.getServerIndexById(ServersUiController.defaultServerId))
                        } else {
                            serverSelectorListView.selectedIndex = 0
                        }

                        serverSelectorListView.positionViewAtIndex(selectedIndex, ListView.Beginning)
                        serverSelectorListView.triggerCurrentItem()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        ServersUiController.setProcessedServerId(ServersUiController.getServerId(proxyServersModel.mapToSource(selectedIndex)))
                    }
                }
            }

            DropDownType {
                id: containerSelector

                signal containerSelectorTextChanged

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                drawerHeight: 0.5

                descriptionText: qsTr("Protocol")
                headerText: qsTr("Protocol")

                listView: RadioListType {
                    id: containerSelectorListView

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
                            },
                            ValueFilter {
                                roleName: "isUnsupportedContainer"
                                value: false
                            }
                        ]
                    }

                    onActivated: {
                        handler()

                        containerSelector.close()
                    }

                    Connections {
                        target: serverSelector

                        function onServerSelectorIndexChanged() {
                            if (!proxyContainersModel.count) {
                                root.shareButtonEnabled = false
                                return
                            }

                            var defaultContainer = proxyContainersModel.mapFromSource(
                                        ServersUiController.serverDefaultContainer(ServersUiController.processedServerId))
                            if (defaultContainer < 0) {
                                defaultContainer = 0
                            }

                            containerSelectorListView.selectedIndex = defaultContainer
                            containerSelectorListView.positionViewAtIndex(defaultContainer, ListView.Beginning)
                            containerSelectorListView.triggerCurrentItem()
                        }
                    }

                    function handler() {
                        if (!proxyContainersModel.count) {
                            root.shareButtonEnabled = false
                            return
                        } else {
                            root.shareButtonEnabled = true
                        }

                        containerSelector.text = selectedText

                        ServersUiController.processedContainerIndex = proxyContainersModel.mapToSource(selectedIndex)

                        fillConnectionTypeModel()

                        if (accessTypeSelector.currentIndex === 1) {
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ServersUiController.processedServerId,
                                                                         ServersUiController.processedContainerIndex)
                            PageController.showBusyIndicator(false)
                        }

                        containerSelector.containerSelectorTextChanged()
                    }

                    function fillConnectionTypeModel() {
                        root.connectionTypesModel = [amneziaConnectionFormat]

                        var index = proxyContainersModel.mapToSource(selectedIndex)

                        if (index === ContainerProps.containerFromString("amnezia-openvpn")) {
                            root.connectionTypesModel.push(openVpnConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-wireguard")) {
                            root.connectionTypesModel.push(wireGuardConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-awg")) {
                            root.connectionTypesModel.push(awgConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-awg2")) {
                            root.connectionTypesModel.push(awgConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-xray")) {
                            root.connectionTypesModel.push(xrayConnectionFormat)
                        }
                    }
                }
            }

            DropDownType {
                id: exportTypeSelector

                property int currentIndex: 0

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                drawerHeight: 0.4375

                visible: accessTypeSelector.currentIndex === 0
                enabled: root.connectionTypesModel.length > 1

                descriptionText: qsTr("Connection format")
                headerText: qsTr("Connection format")

                listView: RadioListType {
                    id: exportTypeSelectorListView

                    onCurrentIndexChanged: {
                        exportTypeSelector.currentIndex = exportTypeSelectorListView.selectedIndex
                        exportTypeSelector.text = exportTypeSelectorListView.selectedText
                    }

                    onModelChanged: {
                        if (exportTypeSelector.currentIndex >= model.length || exportTypeSelector.currentIndex < 0) {
                            exportTypeSelector.currentIndex = 0
                        }
                        selectedIndex = exportTypeSelector.currentIndex
                        if (model.length > 0 && model[selectedIndex] && model[selectedIndex].name !== undefined) {
                            exportTypeSelectorListView.selectedText = model[selectedIndex].name
                            exportTypeSelector.text = model[selectedIndex].name
                        } else {
                            exportTypeSelectorListView.selectedText = ""
                            exportTypeSelector.text = ""
                        }
                    }

                    model: root.connectionTypesModel
                    currentIndex: 0

                    Connections {
                        target: containerSelector

                        function onContainerSelectorTextChanged() {
                            if (exportTypeSelector.currentIndex >= root.connectionTypesModel.length) {
                                exportTypeSelectorListView.selectedIndex = 0
                                exportTypeSelector.currentIndex = 0
                                exportTypeSelector.text = root.connectionTypesModel[0].name
                            }
                        }
                    }

                    onActivated: {
                        exportTypeSelector.text = exportTypeSelectorListView.selectedText
                        exportTypeSelector.currentIndex = exportTypeSelectorListView.selectedIndex
                        exportTypeSelector.close()
                    }
                }
            }

            ButtonType {
                id: shareButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                enabled: shareButtonEnabled
                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share")
                leftImageSource: "qrc:/images/controls/share-2.svg"

                onClicked: {
                    if (clientNameTextField.textField.text !== "") {
                        ExportController.generateConfig(root.connectionTypesModel[exportTypeSelector.currentIndex].type)
                    }
                }
            }

            SectionHeaderType {
                id: usersHeader

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                visible: accessTypeSelector.currentIndex === 1 && !root.isSearchBarVisible

                headerText: qsTr("Users")
                actionButtonImage: "qrc:/images/controls/search.svg"

                onActionClicked: {
                    root.isSearchBarVisible = true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                visible: accessTypeSelector.currentIndex === 1 && root.isSearchBarVisible

                spacing: 16

                SearchFieldType {
                    id: searchTextField

                    Layout.fillWidth: true

                    Keys.onEscapePressed: {
                        searchTextField.text = ""
                        root.isSearchBarVisible = false
                    }

                    function navigateTo() {
                        if (searchTextField.text === "") {
                            root.isSearchBarVisible = false
                        }
                    }

                    onActiveFocusChanged: {
                        if (!searchTextField.activeFocus) {
                            searchTextField.navigateTo()
                        }
                    }

                    Keys.onEnterPressed: { navigateTo() }
                    Keys.onReturnPressed: { navigateTo() }
                }

                IconButtonType {
                    id: closeSearchButton

                    icon.source: "qrc:/images/controls/close.svg"

                    function clickedFunc() {
                        searchTextField.text = ""
                        root.isSearchBarVisible = false
                    }

                    onClicked: clickedFunc()
                }
            }

            ListView {
                id: clientsListView

                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                Layout.bottomMargin: 16

                visible: accessTypeSelector.currentIndex === 1

                function escapeRe(s) { return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') }

                property bool freezeFilter: false

                model: SortFilterProxyModel {
                    id: proxyClientManagementModel
                    sourceModel: ClientManagementModel
                    filters: RegExpFilter {
                        roleName: "clientName"
                        enabled: !clientsListView.freezeFilter
                        pattern: ".*" + clientsListView.escapeRe(searchTextField.text) + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                }

                clip: true
                interactive: false
                reuseItems: true

                delegate: Item {
                    id: clientDelegate

                    required property int index
                    required property var model

                    implicitWidth: clientsListView.width
                    implicitHeight: clientFocusItem.implicitHeight

                    ListItemType {
                        id: clientFocusItem

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        text: clientDelegate.model.clientName
                        trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                        onClicked: clientInfoDrawer.show(clientDelegate.index, clientDelegate.model)
                    }
                }
            }
        }
    }
}
