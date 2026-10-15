import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ContainersModelFilters 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var containersDropDownRef: null

    property var apiAvailableProtocols: []
    property string apiCurrentProtocol: ""

    readonly property bool isApiProtocolSelectionVisible: ServersUiController.isDefaultServerFromApi && root.apiAvailableProtocols.length > 0
    readonly property bool isOutdatedAwgWarningVisible: ServersUiController.defaultServerHasOutdatedAwgContainer

    function updateApiProtocolState() {
        if (ServersUiController.isDefaultServerFromApi) {
            root.apiAvailableProtocols = SubscriptionUiController.availableProtocols(ServersUiController.defaultServerId)
            root.apiCurrentProtocol = SubscriptionUiController.currentProtocol(ServersUiController.defaultServerId)
        } else {
            root.apiAvailableProtocols = []
            root.apiCurrentProtocol = ""
        }
    }

    function openServerInfo() {
        ServersUiController.setProcessedServerId(ServersUiController.defaultServerId)

        if (ServersUiController.isServerFromApi(ServersUiController.processedServerId)) {
            if (ServersUiController.isServerCountrySelectionAvailable(ServersUiController.processedServerId)) {
                PageController.goToPage(PageEnum.PageSettingsApiAvailableCountries)
            } else {
                PageController.showBusyIndicator(true)
                let result = SubscriptionUiController.getAccountInfo(ServersUiController.processedServerId, false)
                PageController.showBusyIndicator(false)
                if (!result) {
                    return
                }

                PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
            }
        } else {
            PageController.goToPage(PageEnum.PageSettingsServerInfo)
        }
    }

    function openProtocolSelection() {
        if (ConnectionController.isConnectionInProgress) {
            PageController.showNotificationMessage(qsTr("Unable change protocol while trying to make an active connection"))
            return
        }
        if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Cannot change protocol during active connection"))
            return
        }
        protocolSelectionDrawer.open()
    }

    function protocolDisplayName(protocol) {
        switch (protocol) {
        case "awg": return "AmneziaWG"
        case "vless": return "VLESS"
        default: return protocol || ""
        }
    }

    Component.onCompleted: {
        root.updateApiProtocolState()
    }

    Connections {
        target: ServersUiController

        function onDefaultServerIdChanged() {
            root.updateApiProtocolState()
        }
    }

    Connections {
        objectName: "pageControllerConnections"

        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            drawer.open()
            if (isContainerInstalled && root.containersDropDownRef) {
                root.containersDropDownRef.open()
            }
        }
    }

    Item {
        objectName: "homeColumnItem"

        anchors.fill: parent
        anchors.bottomMargin: collapsedBar.height

        ColumnLayout {
            objectName: "homeColumnLayout"

            anchors.fill: parent
            anchors.topMargin: 12 + PageController.safeAreaTopMargin
            anchors.bottomMargin: 12

            spacing: 0

            HomeInfoButton {
                id: loggingButton
                objectName: "loggingButton"

                property bool isLoggingEnabled: SettingsController.isLoggingEnabled

                Layout.alignment: Qt.AlignHCenter

                visible: isLoggingEnabled ? true : false
                largeText: true
                text: qsTr("Logging enabled")

                onClicked: {
                    PageController.goToPage(PageEnum.PageSettingsLogging)
                }
            }

            HomeInfoButton {
                id: devGatewayButton
                objectName: "devGatewayButton"

                property bool isDevGatewayEnabled: SettingsController.isDevGatewayEnv

                Layout.alignment: Qt.AlignHCenter

                visible: SettingsController.isDevModeEnabled && isDevGatewayEnabled
                largeText: true
                text: qsTr("Dev gateway enabled")

                onClicked: {
                    PageController.goToPage(PageEnum.PageDevMenu)
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 16
                Layout.bottomMargin: 28

                ConnectButton {
                    id: connectButton
                    objectName: "connectButton"

                    anchors.centerIn: parent
                    width: Math.min(implicitWidth, parent.width, parent.height)
                    height: width
                }
            }

            HomeInfoButton {
                id: splitTunnelingButton
                objectName: "splitTunnelingButton"

                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width - 32

                property bool isSplitTunnelingEnabled: IpSplitTunnelingController.isSplitTunnelingEnabled || AppSplitTunnelingController.isSplitTunnelingEnabled ||
                                                       ServersUiController.isDefaultServerDefaultContainerHasSplitTunneling

                text: isSplitTunnelingEnabled ? qsTr("Split tunneling enabled") : qsTr("Split tunneling disabled")

                leftImageSource: "qrc:/images/controls/split-tunneling.svg"
                rightImageSource: "qrc:/images/controls/chevron-down.svg"

                onClicked: {
                    homeSplitTunnelingDrawer.open()
                }

                HomeSplitTunnelingDrawer {
                    id: homeSplitTunnelingDrawer
                    objectName: "homeSplitTunnelingDrawer"

                    parent: root
                }
            }

            PromoBanner {
                id: promoBanner

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                visible: ServersUiController.isAdVisible
                text: ServersUiController.adHeader
                subtitle: ServersUiController.adDescription

                onClicked: Qt.openUrlExternally(ServersUiController.serverAdEndpoint(ServersUiController.defaultServerId))
            }
        }
    }

    Item {
        id: collapsedBar
        objectName: "collapsedBar"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: drawer.contentTopOffset + collapsedSummary.height

        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -border.width

            topLeftRadius: 16
            topRightRadius: 16
            color: drawer.defaultColor
            border.color: drawer.borderColor
            border.width: 1
        }

        SheetGrabberType {
            anchors.top: parent.top
            anchors.topMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter
        }

        MouseArea {
            id: collapsedBarMouseArea

            property real pressY: 0
            property bool dragStarted: false

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            preventStealing: true

            onPressed: function(mouse) {
                pressY = mouse.y
                dragStarted = false
            }
            onPositionChanged: function(mouse) {
                if (!pressed) {
                    return
                }
                if (!dragStarted && pressY - mouse.y > Application.styleHints.startDragDistance) {
                    dragStarted = true
                    drawer.beginDrag()
                }
                if (dragStarted) {
                    drawer.dragTo(drawer.closedY + mouse.y - pressY)
                }
            }
            onReleased: {
                if (dragStarted) {
                    drawer.endDrag()
                }
            }
            onClicked: {
                if (!dragStarted) {
                    drawer.open()
                }
            }
        }

        HomeServerSummary {
            id: collapsedSummary

            anchors.top: parent.top
            anchors.topMargin: drawer.contentTopOffset
            anchors.left: parent.left
            anchors.right: parent.right

            expanded: false
            pressed: collapsedBarMouseArea.pressed
            outdatedWarningVisible: root.isOutdatedAwgWarningVisible
            protocolSelectionVisible: root.isApiProtocolSelectionVisible
            protocolSelectable: root.apiAvailableProtocols.length > 1
            protocolText: root.protocolDisplayName(root.apiAvailableProtocols.length > 1 ? root.apiCurrentProtocol : root.apiAvailableProtocols[0])

            onExpandRequested: drawer.open()
            onServerInfoRequested: root.openServerInfo()
            onProtocolSelectionRequested: root.openProtocolSelection()
        }
    }

    SheetType {
        id: drawer
        objectName: "drawerProtocol"

        bottomY: root.height
        closedY: root.height - collapsedBar.height
        borderColor: AmneziaStyle.color.borderSoft
        expandedHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : screen.height * 0.77

        Item {
            objectName: "ProtocolDrawerCollapsedContent"

            anchors.fill: parent

            HomeServerSummary {
                id: expandedSummary

                anchors.left: parent.left
                anchors.right: parent.right

                expanded: true
                protocolSelectionVisible: root.isApiProtocolSelectionVisible
                protocolSelectable: root.apiAvailableProtocols.length > 1
                protocolText: root.protocolDisplayName(root.apiAvailableProtocols.length > 1 ? root.apiCurrentProtocol : root.apiAvailableProtocols[0])

                onServerInfoRequested: root.openServerInfo()
                onProtocolSelectionRequested: root.openProtocolSelection()
            }

            ColumnLayout {
                id: serversMenuHeader
                objectName: "serversMenuHeader"

                anchors.top: expandedSummary.bottom
                anchors.right: parent.right
                anchors.left: parent.left

                spacing: 0

                ButtonType {
                    id: containersDropDown
                    objectName: "containersDropDown"

                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: parent.width - 32
                    Layout.topMargin: 16

                    visible: !ServersUiController.isDefaultServerFromApi

                    text: ServersUiController.defaultServerDefaultContainerName
                    rightImageSource: "qrc:/images/controls/chevron-down.svg"

                    onClicked: containersSheet.open()
                }

                BodyTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: containersDropDown.visible ? 24 : 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: qsTr("Servers")
                    color: AmneziaStyle.color.textTertiary
                }
            }

            ServersListView {
                id: serversMenuContent
                objectName: "serversMenuContent"

                anchors.top: serversMenuHeader.bottom
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.topMargin: 12

                onServerSettingsOpened: drawer.close()
            }
        }
    }

    SheetType {
        id: containersSheet
        objectName: "containersSheet"

        expandedHeight: windowHeight * 0.9

        Component.onCompleted: root.containersDropDownRef = containersSheet

        ColumnLayout {
            anchors.fill: parent

            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                IconButtonType {
                    x: 16
                    y: 4

                    size: IconButtonType.Small
                    icon.source: "qrc:/images/controls/arrow-left.svg"
                    Accessible.name: qsTr("Back")

                    onClicked: containersSheet.close()
                }
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("VPN protocol")
                descriptionText: qsTr("Select an installed protocol or add a new one")
            }

            HomeContainersListView {
                id: containersListView
                objectName: "containersListView"

                Layout.fillWidth: true
                Layout.fillHeight: true

                bottomMargin: 16 + PageController.safeAreaBottomMargin

                onActivated: containersSheet.close()

                Connections {
                    objectName: "rowLayoutConnections"

                    target: ServersUiController

                    function onDefaultServerIdChanged() {
                        containersListView.updateContainersModelFilters()
                    }
                }

                function updateContainersModelFilters() {
                    if (ServersUiController.isServerHasWriteAccess(ServersUiController.defaultServerId)) {
                        proxyDefaultServerContainersModel.filters = ContainersModelFilters.getWriteAccessProtocolsListFilters()
                    } else {
                        proxyDefaultServerContainersModel.filters = ContainersModelFilters.getReadAccessProtocolsListFilters()
                    }
                }

                model: SortFilterProxyModel {
                    id: proxyDefaultServerContainersModel
                    sourceModel: DefaultServerContainersModel

                    sorters: [
                        RoleSorter { roleName: "isInstalled"; sortOrder: Qt.DescendingOrder }
                    ]
                }

                Component.onCompleted: updateContainersModelFilters()
            }
        }
    }

    SheetType {
        id: protocolSelectionDrawer
        objectName: "protocolSelectionDrawer"

        expandedHeight: windowHeight * 0.5

        ColumnLayout {
            id: protocolDrawerContainer

            anchors.fill: parent

            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                IconButtonType {
                    id: protocolDrawerBackButton

                    x: 16
                    y: 4

                    size: IconButtonType.Small
                    icon.source: "qrc:/images/controls/arrow-left.svg"
                    Accessible.name: qsTr("Back")

                    onClicked: protocolSelectionDrawer.close()
                }
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("VPN protocol")
            }

            ListViewType {
                id: protocolDrawerListView

                Layout.fillWidth: true
                Layout.fillHeight: true

                leftMargin: 16
                rightMargin: 16
                bottomMargin: 16 + PageController.safeAreaBottomMargin

                model: root.apiAvailableProtocols

                ButtonGroup {
                    id: protocolDrawerButtonGroup
                }

                delegate: RadioItemType {
                    id: protocolDrawerRadioButton

                    width: protocolDrawerListView.width - protocolDrawerListView.leftMargin - protocolDrawerListView.rightMargin

                    showSeparator: index < protocolDrawerListView.count - 1

                    text: root.protocolDisplayName(modelData)

                    ButtonGroup.group: protocolDrawerButtonGroup

                    checkable: !ConnectionController.isConnected
                    checked: modelData === root.apiCurrentProtocol

                    onClicked: {
                        protocolSelectionDrawer.close()

                        if (modelData === root.apiCurrentProtocol) {
                            return
                        }

                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot change protocol during active connection"))
                            return
                        }

                        PageController.showBusyIndicator(true)
                        ServersUiController.setProcessedServerId(ServersUiController.defaultServerId)
                        SubscriptionUiController.setCurrentProtocol(ServersUiController.defaultServerId, modelData)
                        if (!SubscriptionUiController.updateServiceFromGateway(ServersUiController.defaultServerId, "", "", true)) {
                            SubscriptionUiController.setCurrentProtocol(ServersUiController.defaultServerId, root.apiCurrentProtocol)
                        }
                        root.updateApiProtocolState()
                        PageController.showBusyIndicator(false)
                    }
                }
            }
        }
    }
}
