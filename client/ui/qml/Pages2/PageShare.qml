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
        ShadowSocks,
        Cloak,
        Xray
    }

    signal revokeConfig(int index)
    onRevokeConfig: function(index) {
        PageController.showBusyIndicator(true)
        ExportController.revokeConfig(index,
                                      ContainersModel.getProcessedContainerIndex(),
                                      ServersModel.getProcessedServerCredentials())
        PageController.showBusyIndicator(false)
        PageController.showNotificationMessage(qsTr("Config revoked"))
    }

    Connections {
        target: ExportController

        function onGenerateConfig(type) {
            PageController.showBusyIndicator(true)

            switch (type) {
            case PageShare.ConfigType.AmneziaConnection: {
                ExportController.generateConnectionConfig(clientNameTextField.textField.text);
                break;
            }
            case PageShare.ConfigType.OpenVpn: {
                ExportController.generateOpenVpnConfig(clientNameTextField.textField.text)
                break
            }
            case PageShare.ConfigType.WireGuard: {
                ExportController.generateWireGuardConfig(clientNameTextField.textField.text)
                break
            }
            case PageShare.ConfigType.Awg: {
                ExportController.generateAwgConfig(clientNameTextField.textField.text)
                break
            }
            case PageShare.ConfigType.ShadowSocks: {
                ExportController.generateShadowSocksConfig()
                break
            }
            case PageShare.ConfigType.Cloak: {
                ExportController.generateCloakConfig()
                break
            }
            case PageShare.ConfigType.Xray: {
                ExportController.generateXrayConfig(clientNameTextField.textField.text)
                break
            }
            }

            PageController.showBusyIndicator(false)
            
            PageController.goToPage(PageEnum.PageShareConnection)
        }

        function onExportErrorOccurred(error) {
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
        id: shadowSocksConnectionFormat
        readonly property string name: qsTr("Shadowsocks native format")
        readonly property int type: PageShare.ConfigType.ShadowSocks
    }
    QtObject {
        id: cloakConnectionFormat
        readonly property string name: qsTr("Cloak native format")
        readonly property int type: PageShare.ConfigType.Cloak
    }
    QtObject {
        id: xrayConnectionFormat
        readonly property string name: qsTr("XRay native format")
        readonly property int type: PageShare.ConfigType.Xray
    }

    FlickableType {
        id: a

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height + 10

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 0

            HeaderTypeWithButton {
                id: header
                Layout.fillWidth: true
                Layout.topMargin: 24

                headerText: qsTr("Share VPN Access")

                actionButtonImage: "qrc:/images/controls/more-vertical.svg"
                actionButtonFunction: function() {
                    shareFullAccessDrawer.openTriggered()
                }

                DrawerType2 {
                    id: shareFullAccessDrawer

                    parent: root

                    anchors.fill: parent
                    expandedHeight: root.height

                    expandedStateContent: ColumnLayout {
                        id: shareFullAccessDrawerContent
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        spacing: 0

                        onImplicitHeightChanged: {
                            shareFullAccessDrawer.expandedHeight = shareFullAccessDrawerContent.implicitHeight + 32
                        }

                        Header2Type {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 16
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            headerText: qsTr("Share full access to the server and VPN")
                            descriptionText: qsTr("Use for your own devices, or share with those you trust to manage the server.")
                        }

                        LabelWithButtonType {
                            id: shareFullAccessButton
                            Layout.fillWidth: true

                            text: qsTr("Share")
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                PageController.goToPage(PageEnum.PageShareFullAccess)
                                shareFullAccessDrawer.closeTriggered()
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

                color: AmneziaStyle.color.onyxBlack
                radius: 16

                RowLayout {
                    id: accessTypeSelectorContent

                    spacing: 0

                    HorizontalRadioButton {
                        id: connectionRadioButton
                        checked: accessTypeSelector.currentIndex === 0

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Connection")

                        onClicked: {
                            accessTypeSelector.currentIndex = 0
                        }

                        Keys.onEnterPressed: this.clicked()
                        Keys.onReturnPressed: this.clicked()
                    }

                    HorizontalRadioButton {
                        id: usersRadioButton
                        checked: accessTypeSelector.currentIndex === 1

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Users")

                        onClicked: {
                            accessTypeSelector.currentIndex = 1
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ContainersModel.getProcessedContainerIndex(),
                                                                         ServersModel.getProcessedServerCredentials())
                            PageController.showBusyIndicator(false)
                        }

                        Keys.onEnterPressed: this.clicked()
                        Keys.onReturnPressed: this.clicked()
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24

                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share VPN access without the ability to manage the server")
                color: AmneziaStyle.color.mutedGray
            }

            TextFieldWithHeaderType {
                id: clientNameTextField
                Layout.fillWidth: true
                Layout.topMargin: 16

                visible: accessTypeSelector.currentIndex === 0

                headerText: qsTr("User name")
                textField.text: "New client"
                textField.maximumLength: 20

                checkEmptyText: true
            }

            DropDownType {
                id: serverSelector

                signal severSelectorIndexChanged
                property int currentIndex: -1

                Layout.fillWidth: true
                Layout.topMargin: 16

                drawerHeight: 0.4375
                drawerParent: root

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
                            },
                            ValueFilter {
                                roleName: "hasInstalledContainers"
                                value: true
                            }
                        ]
                    }

                    clickedFunction: function() {
                        handler()

                        if (serverSelector.currentIndex !== serverSelectorListView.selectedIndex) {
                            serverSelector.currentIndex = serverSelectorListView.selectedIndex
                            serverSelector.severSelectorIndexChanged()
                        }

                        serverSelector.closeTriggered()
                    }

                    Component.onCompleted: {
                        if (ServersModel.isDefaultServerHasWriteAccess() && ServersModel.getDefaultServerData("hasInstalledContainers")) {
                            serverSelectorListView.selectedIndex = proxyServersModel.mapFromSource(ServersModel.defaultIndex)
                        } else {
                            serverSelectorListView.selectedIndex = 0
                        }

                        serverSelectorListView.positionViewAtIndex(selectedIndex, ListView.Beginning)
                        serverSelectorListView.triggerCurrentItem()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        ServersModel.processedIndex = proxyServersModel.mapToSource(selectedIndex)
                    }
                }
            }

            DropDownType {
                id: protocolSelector

                Layout.fillWidth: true
                Layout.topMargin: 16

                drawerHeight: 0.5
                drawerParent: root

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

                    clickedFunction: function() {
                        handler()

                        protocolSelector.closeTriggered()
                    }

                    Connections {
                        target: serverSelector

                        function onSeverSelectorIndexChanged() {
                            var defaultContainer = proxyContainersModel.mapFromSource(ServersModel.getProcessedServerData("defaultContainer"))
                            protocolSelectorListView.selectedIndex = defaultContainer
                            protocolSelectorListView.positionViewAtIndex(selectedIndex, ListView.Beginning)
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

                        ContainersModel.setProcessedContainerIndex(proxyContainersModel.mapToSource(selectedIndex))

                        fillConnectionTypeModel()

                        if (exportTypeSelector.currentIndex >= root.connectionTypesModel.length) {
                            exportTypeSelector.currentIndex = 0
                            exportTypeSelector.text = root.connectionTypesModel[0].name
                        }

                        if (accessTypeSelector.currentIndex === 1) {
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ContainersModel.getProcessedContainerIndex(),
                                                                         ServersModel.getProcessedServerCredentials())
                            PageController.showBusyIndicator(false)
                        }
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
                        } else if (index === ContainerProps.containerFromString("amnezia-shadowsocks")) {
                            root.connectionTypesModel.push(openVpnConnectionFormat)
                            root.connectionTypesModel.push(shadowSocksConnectionFormat)
                        } else if (index === ContainerProps.containerFromString("amnezia-openvpn-cloak")) {
                            root.connectionTypesModel.push(openVpnConnectionFormat)
                            root.connectionTypesModel.push(shadowSocksConnectionFormat)
                            root.connectionTypesModel.push(cloakConnectionFormat)
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
                Layout.topMargin: 16

                drawerHeight: 0.4375
                drawerParent: root

                visible: accessTypeSelector.currentIndex === 0
                enabled: root.connectionTypesModel.length > 1

                descriptionText: qsTr("Connection format")
                headerText: qsTr("Connection format")

                listView: ListViewWithRadioButtonType {
                    id: exportTypeSelectorListView

                    onCurrentIndexChanged: {
                        exportTypeSelector.currentIndex = exportTypeSelectorListView.selectedIndex
                        exportTypeSelector.text = exportTypeSelectorListView.selectedText
                    }

                    rootWidth: root.width

                    imageSource: "qrc:/images/controls/check.svg"

                    model: root.connectionTypesModel
                    currentIndex: 0

                    clickedFunction: function() {
                        exportTypeSelector.text = exportTypeSelectorListView.selectedText
                        exportTypeSelector.currentIndex = exportTypeSelectorListView.selectedIndex
                        exportTypeSelector.closeTriggered()
                    }

                    Component.onCompleted: {
                        exportTypeSelector.text = exportTypeSelectorListView.selectedText
                        exportTypeSelector.currentIndex = exportTypeSelectorListView.selectedIndex
                    }
                }
            }

            BasicButtonType {
                id: shareButton

                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.bottomMargin: 32

                enabled: shareButtonEnabled
                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share")
                leftImageSource: "qrc:/images/controls/share-2.svg"

                clickedFunc: function(){
                    if (clientNameTextField.textField.text !== "") {
                        ExportController.generateConfig(root.connectionTypesModel[exportTypeSelector.currentIndex].type)
                    }
                }
            }

            Header2Type {
                id: usersHeader
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

                    textField.placeholderText: qsTr("Search")

                    Keys.onEscapePressed: {
                        root.isSearchBarVisible = false
                    }

                    function navigateTo() {
                        if (searchTextField.textField.text === "") {
                            root.isSearchBarVisible = false
                        }
                    }

                    Keys.onTabPressed: { navigateTo() }
                    Keys.onEnterPressed: { navigateTo() }
                    Keys.onReturnPressed: { navigateTo() }
                }

                ImageButtonType {
                    id: closeSearchButton
                    image: "qrc:/images/controls/close.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    function clickedFunc() {
                        root.isSearchBarVisible = false
                    }

                    onClicked: clickedFunc()
                    Keys.onEnterPressed: clickedFunc()
                    Keys.onReturnPressed: clickedFunc()
                }
            }

            ListView {
                id: clientsListView
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height

                visible: accessTypeSelector.currentIndex === 1

                property bool isFocusable: true

                model: SortFilterProxyModel {
                    id: proxyClientManagementModel
                    sourceModel: ClientManagementModel
                    filters: RegExpFilter {
                        roleName: "clientName"
                        pattern: ".*" + searchTextField.textField.text + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                }

                clip: true
                interactive: false
                reuseItems: true

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
                            id: clientFocusItem
                            Layout.fillWidth: true

                            text: clientName
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                clientInfoDrawer.openTriggered()
                            }
                        }

                        DividerType {}

                        DrawerType2 {
                            id: clientInfoDrawer

                            parent: root

                            width: root.width
                            height: root.height

                            expandedStateContent: ColumnLayout {
                                id: expandedStateContent
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 16
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16

                                onImplicitHeightChanged: {
                                    clientInfoDrawer.expandedHeight = expandedStateContent.implicitHeight + 32
                                }

                                Header2TextType {
                                    Layout.maximumWidth: parent.width
                                    Layout.bottomMargin: 24

                                    text: clientName
                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Qt.ElideRight
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: creationDate
                                    Layout.maximumWidth: parent.width

                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Qt.ElideRight

                                    text: qsTr("Creation date: %1").arg(creationDate)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: latestHandshake
                                    Layout.maximumWidth: parent.width

                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Qt.ElideRight

                                    text: qsTr("Latest handshake: %1").arg(latestHandshake)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: dataReceived
                                    Layout.maximumWidth: parent.width

                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Qt.ElideRight

                                    text: qsTr("Data received: %1").arg(dataReceived)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: dataSent
                                    Layout.maximumWidth: parent.width

                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Qt.ElideRight

                                    text: qsTr("Data sent: %1").arg(dataSent)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: allowedIps
                                    Layout.maximumWidth: parent.width

                                    wrapMode: Text.Wrap

                                    text: qsTr("Allowed IPs: %1").arg(allowedIps)
                                }

                                BasicButtonType {
                                    id: renameButton
                                    Layout.fillWidth: true
                                    Layout.topMargin: 24

                                    defaultColor: AmneziaStyle.color.transparent
                                    hoveredColor: AmneziaStyle.color.translucentWhite
                                    pressedColor: AmneziaStyle.color.sheerWhite
                                    disabledColor: AmneziaStyle.color.mutedGray
                                    textColor: AmneziaStyle.color.paleGray
                                    borderWidth: 1

                                    text: qsTr("Rename")

                                    clickedFunc: function() {
                                        clientNameEditDrawer.openTriggered()
                                    }

                                    DrawerType2 {
                                        id: clientNameEditDrawer

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
                                                id: clientNameEditor
                                                Layout.fillWidth: true
                                                headerText: qsTr("Client name")
                                                textField.text: clientName
                                                textField.maximumLength: 20
                                                checkEmptyText: true
                                            }

                                            BasicButtonType {
                                                id: saveButton

                                                Layout.fillWidth: true

                                                text: qsTr("Save")

                                                clickedFunc: function() {
                                                    if (clientNameEditor.textField.text === "") {
                                                        return
                                                    }

                                                    if (clientNameEditor.textField.text !== clientName) {
                                                        PageController.showBusyIndicator(true)
                                                        ExportController.renameClient(index,
                                                                                      clientNameEditor.textField.text,
                                                                                      ContainersModel.getProcessedContainerIndex(),
                                                                                      ServersModel.getProcessedServerCredentials())
                                                        PageController.showBusyIndicator(false)
                                                        clientNameEditDrawer.closeTriggered()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                BasicButtonType {
                                    id: revokeButton
                                    Layout.fillWidth: true
                                    Layout.topMargin: 8

                                    defaultColor: AmneziaStyle.color.transparent
                                    hoveredColor: AmneziaStyle.color.translucentWhite
                                    pressedColor: AmneziaStyle.color.sheerWhite
                                    disabledColor: AmneziaStyle.color.mutedGray
                                    textColor: AmneziaStyle.color.paleGray
                                    borderWidth: 1

                                    text: qsTr("Revoke")

                                    clickedFunc: function() {
                                        var headerText = qsTr("Revoke the config for a user - %1?").arg(clientName)
                                        var descriptionText = qsTr("The user will no longer be able to connect to your server.")
                                        var yesButtonText = qsTr("Continue")
                                        var noButtonText = qsTr("Cancel")

                                        var yesButtonFunction = function() {
                                            clientInfoDrawer.closeTriggered()
                                            root.revokeConfig(index)
                                        }
                                        var noButtonFunction = function() {
                                        }

                                        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}
