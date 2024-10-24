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

    defaultActiveFocusItem: clientNameTextField.textField

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
            shareConnectionDrawer.headerText = qsTr("Connection to ") + serverSelector.text
            shareConnectionDrawer.configContentHeaderText = qsTr("File with connection settings to ") + serverSelector.text

            shareConnectionDrawer.open()
            shareConnectionDrawer.contentVisible = false
            PageController.showBusyIndicator(true)

            switch (type) {
            case PageShare.ConfigType.AmneziaConnection: {
                ExportController.generateConnectionConfig(clientNameTextField.textFieldText);
                break;
            }
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
            case PageShare.ConfigType.Awg: {
                ExportController.generateAwgConfig(clientNameTextField.textFieldText)
                shareConnectionDrawer.configCaption = qsTr("Save AmneziaWG config")
                shareConnectionDrawer.configExtension = ".conf"
                shareConnectionDrawer.configFileName = "amnezia_for_awg"
                break
            }
            case PageShare.ConfigType.ShadowSocks: {
                ExportController.generateShadowSocksConfig()
                shareConnectionDrawer.configCaption = qsTr("Save Shadowsocks config")
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
            case PageShare.ConfigType.Xray: {
                ExportController.generateXrayConfig()
                shareConnectionDrawer.configCaption = qsTr("Save XRay config")
                shareConnectionDrawer.configExtension = ".json"
                shareConnectionDrawer.configFileName = "amnezia_for_xray"
                break
            }
            }

            PageController.showBusyIndicator(false)
        }

        function onExportErrorOccurred(error) {
            shareConnectionDrawer.close()

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
        property string name: qsTr("For the AmneziaVPN app")
        property var type: PageShare.ConfigType.AmneziaConnection
    }
    QtObject {
        id: openVpnConnectionFormat
        property string name: qsTr("OpenVPN native format")
        property var type: PageShare.ConfigType.OpenVpn
    }
    QtObject {
        id: wireGuardConnectionFormat
        property string name: qsTr("WireGuard native format")
        property var type: PageShare.ConfigType.WireGuard
    }
    QtObject {
        id: awgConnectionFormat
        property string name: qsTr("AmneziaWG native format")
        property var type: PageShare.ConfigType.Awg
    }
    QtObject {
        id: shadowSocksConnectionFormat
        property string name: qsTr("Shadowsocks native format")
        property var type: PageShare.ConfigType.ShadowSocks
    }
    QtObject {
        id: cloakConnectionFormat
        property string name: qsTr("Cloak native format")
        property var type: PageShare.ConfigType.Cloak
    }
    QtObject {
        id: xrayConnectionFormat
        property string name: qsTr("XRay native format")
        property var type: PageShare.ConfigType.Xray
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

            Item {
                id: focusItem
                KeyNavigation.tab: header.actionButton
                onFocusChanged: {
                    if (focusItem.activeFocus) {
                        a.contentY = 0
                    }
                }
            }

            HeaderType {
                id: header
                Layout.fillWidth: true
                Layout.topMargin: 24

                headerText: qsTr("Share VPN Access")

                actionButtonImage: "qrc:/images/controls/more-vertical.svg"
                actionButtonFunction: function() {
                    shareFullAccessDrawer.open()
                }

                KeyNavigation.tab: connectionRadioButton

                DrawerType2 {
                    id: shareFullAccessDrawer

                    parent: root

                    anchors.fill: parent
                    expandedHeight: root.height
                    onClosed: {
                        if (!GC.isMobile()) {
                            clientNameTextField.textField.forceActiveFocus()
                        }
                    }

                    expandedContent: ColumnLayout {
                        id: shareFullAccessDrawerContent
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: 16

                        spacing: 0

                        onImplicitHeightChanged: {
                            shareFullAccessDrawer.expandedHeight = shareFullAccessDrawerContent.implicitHeight + 32
                        }

                        Connections {
                            target: shareFullAccessDrawer
                            enabled: !GC.isMobile()
                            function onOpened() {
                                focusItem.forceActiveFocus()
                            }
                        }

                        Header2Type {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 16
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            headerText: qsTr("Share full access to the server and VPN")
                            descriptionText: qsTr("Use for your own devices, or share with those you trust to manage the server.")
                        }

                        Item {
                            id: focusItem
                            KeyNavigation.tab: shareFullAccessButton.rightButton
                        }

                        LabelWithButtonType {
                            id: shareFullAccessButton
                            Layout.fillWidth: true

                            text: qsTr("Share")
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"
                            KeyNavigation.tab: focusItem

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

                        KeyNavigation.tab: usersRadioButton

                        onClicked: {
                            accessTypeSelector.currentIndex = 0
                            if (!GC.isMobile()) {
                                clientNameTextField.textField.forceActiveFocus()
                            }
                        }
                    }

                    HorizontalRadioButton {
                        id: usersRadioButton
                        checked: accessTypeSelector.currentIndex === 1

                        implicitWidth: (root.width - 32) / 2
                        text: qsTr("Users")

                        KeyNavigation.tab: accessTypeSelector.currentIndex === 0 ? clientNameTextField.textField : serverSelector

                        onClicked: {
                            accessTypeSelector.currentIndex = 1
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ContainersModel.getProcessedContainerIndex(),
                                                                         ServersModel.getProcessedServerCredentials())
                            PageController.showBusyIndicator(false)
                            focusItem.forceActiveFocus()
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
                color: AmneziaStyle.color.mutedGray
            }

            TextFieldWithHeaderType {
                id: clientNameTextField
                Layout.fillWidth: true
                Layout.topMargin: 16

                visible: accessTypeSelector.currentIndex === 0

                headerText: qsTr("User name")
                textFieldText: "New client"
                textField.maximumLength: 20

                checkEmptyText: true

                KeyNavigation.tab: serverSelector

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

                        if (serverSelector.currentIndex !== serverSelectorListView.currentIndex) {
                            serverSelector.currentIndex = serverSelectorListView.currentIndex
                            serverSelector.severSelectorIndexChanged()
                        }

                        serverSelector.close()
                    }

                    Component.onCompleted: {
                        if (ServersModel.isDefaultServerHasWriteAccess() && ServersModel.getDefaultServerData("hasInstalledContainers")) {
                            serverSelectorListView.currentIndex = proxyServersModel.mapFromSource(ServersModel.defaultIndex)
                        } else {
                            serverSelectorListView.currentIndex = 0
                        }

                        serverSelectorListView.triggerCurrentItem()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        ServersModel.processedIndex = proxyServersModel.mapToSource(currentIndex)
                    }
                }

                KeyNavigation.tab: protocolSelector
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

                    currentIndex: 0

                    clickedFunction: function() {
                        handler()

                        protocolSelector.close()
                    }

                    Connections {
                        target: serverSelector

                        function onSeverSelectorIndexChanged() {
                            var defaultContainer = proxyContainersModel.mapFromSource(ServersModel.getProcessedServerData("defaultContainer"))
                            protocolSelectorListView.currentIndex = defaultContainer
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

                        ContainersModel.setProcessedContainerIndex(proxyContainersModel.mapToSource(currentIndex))

                        fillConnectionTypeModel()

                        if (accessTypeSelector.currentIndex === 1) {
                            PageController.showBusyIndicator(true)
                            ExportController.updateClientManagementModel(ContainersModel.getProcessedContainerIndex(),
                                                                         ServersModel.getProcessedServerCredentials())
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

                KeyNavigation.tab: accessTypeSelector.currentIndex === 0 ?
                                       exportTypeSelector :
                                       isSearchBarVisible ?
                                           searchTextField.textField :
                                           usersHeader.actionButton
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
                        exportTypeSelector.close()
                    }

                    Component.onCompleted: {
                        exportTypeSelector.text = selectedText
                        exportTypeSelector.currentIndex = currentIndex
                    }
                }

                KeyNavigation.tab: shareButton

            }

            BasicButtonType {
                id: shareButton

                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.bottomMargin: 32

                enabled: shareButtonEnabled
                visible: accessTypeSelector.currentIndex === 0

                text: qsTr("Share")
                imageSource: "qrc:/images/controls/share-2.svg"                

                Keys.onTabPressed: lastItemTabClicked(focusItem)

                parentFlickable: a

                clickedFunc: function(){
                    if (clientNameTextField.textFieldText !== "") {
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

                Keys.onTabPressed: clientsListView.model.count > 0 ?
                                       clientsListView.forceActiveFocus() :
                                       lastItemTabClicked(focusItem)

            }

            RowLayout {
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                visible: accessTypeSelector.currentIndex === 1 && root.isSearchBarVisible

                TextFieldWithHeaderType {
                    id: searchTextField
                    Layout.fillWidth: true

                    textFieldPlaceholderText: qsTr("Search")

                    Connections {
                        target: root
                        function onIsSearchBarVisibleChanged() {
                            if (root.isSearchBarVisible) {
                                searchTextField.textField.forceActiveFocus()
                            } else {
                                searchTextField.textFieldText = ""
                                if (!GC.isMobile()) {
                                    usersHeader.actionButton.forceActiveFocus()
                                }
                            }
                        }
                    }

                    Keys.onEscapePressed: {
                        root.isSearchBarVisible = false
                    }

                    function navigateTo() {
                        if (GC.isMobile()) {
                            focusItem.forceActiveFocus()
                            return;
                        }

                        if (searchTextField.textFieldText === "") {
                            root.isSearchBarVisible = false
                            usersHeader.actionButton.forceActiveFocus()
                        } else {
                            closeSearchButton.forceActiveFocus()
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

                    Keys.onTabPressed: {
                        if (!GC.isMobile()) {
                            if (clientsListView.model.count > 0) {
                                clientsListView.forceActiveFocus()
                            } else {
                                lastItemTabClicked(focusItem)
                            }
                        }
                    }

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

                activeFocusOnTab: true
                focus: true
                Keys.onTabPressed: {
                    if (!GC.isMobile()) {
                        if (currentIndex < this.count - 1) {
                            this.incrementCurrentIndex()
                            currentItem.focusItem.forceActiveFocus()
                        } else {
                            this.currentIndex = 0
                            lastItemTabClicked(focusItem)
                        }
                    }
                }

                onActiveFocusChanged: {
                    if (focus && !GC.isMobile()) {
                        currentIndex = 0
                        currentItem.focusItem.forceActiveFocus()
                    }
                }

                onCurrentIndexChanged: {
                    if (currentItem) {
                        if (currentItem.y < a.contentY) {
                            a.contentY = currentItem.y
                        } else if (currentItem.y + currentItem.height + clientsListView.y > a.contentY + a.height) {
                            a.contentY = currentItem.y + clientsListView.y + currentItem.height - a.height
                        }
                    }
                }

                delegate: Item {
                    implicitWidth: clientsListView.width
                    implicitHeight: delegateContent.implicitHeight

                    property alias focusItem: clientFocusItem.rightButton

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
                                clientInfoDrawer.open()
                            }
                        }

                        DividerType {}

                        DrawerType2 {
                            id: clientInfoDrawer

                            parent: root

                            onClosed: {
                                if (!GC.isMobile()) {
                                    focusItem.forceActiveFocus()
                                }
                            }

                            width: root.width
                            height: root.height

                            expandedContent: ColumnLayout {
                                id: expandedContent
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 16
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16

                                onImplicitHeightChanged: {
                                    clientInfoDrawer.expandedHeight = expandedContent.implicitHeight + 32
                                }

                                Connections {
                                    target: clientInfoDrawer
                                    enabled: !GC.isMobile()
                                    function onOpened() {
                                        focusItem1.forceActiveFocus()
                                    }
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
                                    Layout.fillWidth: true

                                    text: qsTr("Creation date: %1").arg(creationDate)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: latestHandshake
                                    Layout.fillWidth: true

                                    text: qsTr("Latest handshake: %1").arg(latestHandshake)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: dataReceived
                                    Layout.fillWidth: true

                                    text: qsTr("Data received: %1").arg(dataReceived)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: dataSent
                                    Layout.fillWidth: true

                                    text: qsTr("Data sent: %1").arg(dataSent)
                                }

                                ParagraphTextType {
                                    color: AmneziaStyle.color.mutedGray
                                    visible: allowedIps
                                    Layout.fillWidth: true

                                    text: qsTr("Allowed IPs: %1").arg(allowedIps)
                                }

                                Item {
                                    id: focusItem1
                                    KeyNavigation.tab: renameButton
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

                                    KeyNavigation.tab: revokeButton

                                    clickedFunc: function() {
                                        clientNameEditDrawer.open()
                                    }

                                    DrawerType2 {
                                        id: clientNameEditDrawer

                                        parent: root

                                        anchors.fill: parent
                                        expandedHeight: root.height * 0.35

                                        onClosed: {
                                            if (!GC.isMobile()) {
                                                focusItem1.forceActiveFocus()
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
                                                target: clientNameEditDrawer
                                                enabled: !GC.isMobile()
                                                function onOpened() {
                                                    clientNameEditor.textField.forceActiveFocus()
                                                }
                                            }

                                            Item {
                                                id: focusItem2
                                                KeyNavigation.tab: clientNameEditor.textField
                                            }

                                            TextFieldWithHeaderType {
                                                id: clientNameEditor
                                                Layout.fillWidth: true
                                                headerText: qsTr("Client name")
                                                textFieldText: clientName
                                                textField.maximumLength: 20
                                                checkEmptyText: true

                                                KeyNavigation.tab: saveButton
                                            }

                                            BasicButtonType {
                                                id: saveButton

                                                Layout.fillWidth: true

                                                text: qsTr("Save")
                                                KeyNavigation.tab: focusItem2

                                                clickedFunc: function() {
                                                    if (clientNameEditor.textFieldText === "") {
                                                        return
                                                    }

                                                    if (clientNameEditor.textFieldText !== clientName) {
                                                        PageController.showBusyIndicator(true)
                                                        ExportController.renameClient(index,
                                                                                      clientNameEditor.textFieldText,
                                                                                      ContainersModel.getProcessedContainerIndex(),
                                                                                      ServersModel.getProcessedServerCredentials())
                                                        PageController.showBusyIndicator(false)
                                                        clientNameEditDrawer.close()
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
                                    KeyNavigation.tab: focusItem1

                                    clickedFunc: function() {
                                        var headerText = qsTr("Revoke the config for a user - %1?").arg(clientName)
                                        var descriptionText = qsTr("The user will no longer be able to connect to your server.")
                                        var yesButtonText = qsTr("Continue")
                                        var noButtonText = qsTr("Cancel")

                                        var yesButtonFunction = function() {
                                            clientInfoDrawer.close()
                                            root.revokeConfig(index)
                                        }
                                        var noButtonFunction = function() {
                                            if (!GC.isMobile()) {
                                                focusItem1.forceActiveFocus()
                                            }
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

    ShareConnectionDrawer {
        id: shareConnectionDrawer

        anchors.fill: parent
        onClosed: {
            if (!GC.isMobile()) {
                clientNameTextField.textField.forceActiveFocus()
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
