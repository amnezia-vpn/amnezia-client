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

    function protocolDisplayName(protocol) {
        switch (protocol) {
        case "awg": return "AmneziaWG"
        case "vless": return "VLESS"
        default: return protocol
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
                root.containersDropDownRef.rootButtonClickedFunction()
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
            anchors.bottomMargin: 16

            BasicButtonType {
                id: loggingButton
                objectName: "loggingButton"

                property bool isLoggingEnabled: SettingsController.isLoggingEnabled

                Layout.alignment: Qt.AlignHCenter

                implicitHeight: 36

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.mutedGray
                borderWidth: 0

                visible: isLoggingEnabled ? true : false
                text: qsTr("Logging enabled")

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()

                onClicked: {
                    PageController.goToPage(PageEnum.PageSettingsLogging)
                }
            }

            BasicButtonType {
                id: devGatewayButton
                objectName: "devGatewayButton"

                property bool isDevGatewayEnabled: SettingsController.isDevGatewayEnv

                Layout.alignment: Qt.AlignHCenter

                implicitHeight: 36

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.mutedGray
                borderWidth: 0

                visible: SettingsController.isDevModeEnabled && isDevGatewayEnabled
                text: qsTr("Dev gateway enabled")

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()

                onClicked: {
                    PageController.goToPage(PageEnum.PageDevMenu)
                }
            }

            ConnectButton {
                id: connectButton
                objectName: "connectButton"

                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
            }

            BasicButtonType {
                id: splitTunnelingButton
                objectName: "splitTunnelingButton"

                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                leftPadding: 16
                rightPadding: 16

                implicitHeight: 36

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.mutedGray
                borderWidth: 0

                buttonTextLabel.lineHeight: 20
                buttonTextLabel.font.pixelSize: 14
                buttonTextLabel.font.weight: 500

                property bool isSplitTunnelingEnabled: IpSplitTunnelingController.isSplitTunnelingEnabled || AppSplitTunnelingController.isSplitTunnelingEnabled ||
                                                       ServersUiController.isDefaultServerDefaultContainerHasSplitTunneling

                text: isSplitTunnelingEnabled ? qsTr("Split tunneling enabled") : qsTr("Split tunneling disabled")

                leftImageSource: isSplitTunnelingEnabled ? "qrc:/images/controls/split-tunneling.svg" : ""
                leftImageColor: ""
                rightImageSource: "qrc:/images/controls/chevron-down.svg"

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()

                onClicked: {
                    homeSplitTunnelingDrawer.open()
                }

                HomeSplitTunnelingDrawer {
                    id: homeSplitTunnelingDrawer
                    objectName: "homeSplitTunnelingDrawer"

                    parent: root
                }
            }

            AdLabel {
                id: adLabel

                Layout.fillWidth: true
                Layout.preferredHeight: adLabel.contentHeight
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 22
            }
        }
    }

    Component {
        id: serverSummaryComponent

        ColumnLayout {
            id: summary
            objectName: "collapsedColumnLayout"

            readonly property bool expanded: parent ? parent.expanded : false
            readonly property bool hovered: parent ? parent.hovered : false
            readonly property bool pressed: parent ? parent.pressed : false
            readonly property bool outdatedWarningVisible: !summary.expanded && root.isOutdatedAwgWarningVisible

            spacing: 0

            DividerType {
                Layout.topMargin: 10
                Layout.fillWidth: false
                Layout.preferredWidth: 20
                Layout.preferredHeight: 2
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            }

            RowLayout {
                objectName: "rowLayout"

                Layout.topMargin: 14
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                spacing: 0

                Header1TextType {
                    id: collapsedButtonHeader
                    objectName: "collapsedButtonHeader"

                    Layout.maximumWidth: drawer.width - 48 - 18 - 12

                    maximumLineCount: 2
                    elide: Qt.ElideRight

                    text: ServersUiController.defaultServerName
                    horizontalAlignment: Qt.AlignHCenter

                    opacity: summary.pressed ? 0.7 : (summary.hovered ? 0.8 : 1)

                    Behavior on opacity {
                        PropertyAnimation { duration: 200 }
                    }
                }

                ImageButtonType {
                    id: collapsedButtonChevron
                    objectName: "collapsedButtonChevron"

                    Layout.leftMargin: 8

                    visible: !summary.expanded

                    hoverEnabled: false
                    backgroundColor: summary.pressed ? pressedColor : (summary.hovered ? hoveredColor : defaultColor)
                    image: "qrc:/images/controls/chevron-down.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    icon.width: 18
                    icon.height: 18
                    backgroundRadius: 16
                    horizontalPadding: 4
                    topPadding: 4
                    bottomPadding: 3

                    Keys.onEnterPressed: this.clicked()
                    Keys.onReturnPressed: this.clicked()

                    onClicked: drawer.open()
                }
            }

            RowLayout {
                objectName: "rowLayoutLabel"
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Layout.topMargin: 8
                Layout.bottomMargin: (summary.outdatedWarningVisible || root.isApiProtocolSelectionVisible) ? 8 : 44
                spacing: 0

                BasicButtonType {
                    enabled: ServersUiController.defaultServerImagePathCollapsed !== ""
                    hoverEnabled: enabled

                    implicitHeight: 36

                    leftPadding: 16
                    rightPadding: 16

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.transparent
                    textColor: AmneziaStyle.color.mutedGray

                    buttonTextLabel.lineHeight: 16
                    buttonTextLabel.font.pixelSize: 13
                    buttonTextLabel.font.weight: 400

                    text: ServersUiController.defaultServerDescriptionCollapsed
                    leftImageSource: ServersUiController.defaultServerImagePathCollapsed
                    leftImageColor: ""
                    changeLeftImageSize: false

                    rightImageSource: hoverEnabled ? "qrc:/images/controls/chevron-down.svg" : ""

                    Keys.onEnterPressed: this.clicked()
                    Keys.onReturnPressed: this.clicked()

                    onClicked: {
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
                }
            }

            WarningType {
                objectName: "outdatedContainerWarning"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                visible: summary.outdatedWarningVisible

                backGroundColor: AmneziaStyle.color.transparent
                iconPath: "qrc:/images/controls/alert-circle.svg"
                imageColor: AmneziaStyle.color.goldenApricot
                textColor: AmneziaStyle.color.goldenApricot
                textString: qsTr("AmneziaWG 2.0 is outdated and no longer supported. Continued use requires a fresh installation of the AmneziaWG 3.1 container.")
            }

            RowLayout {
                objectName: "protocolRowLayout"
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Layout.bottomMargin: 44
                spacing: 0

                visible: root.isApiProtocolSelectionVisible

                BasicButtonType {
                    id: protocolButton
                    objectName: "protocolButton"

                    enabled: root.apiAvailableProtocols.length > 1
                    hoverEnabled: enabled

                    implicitHeight: 36

                    leftPadding: 16
                    rightPadding: 16

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.transparent
                    textColor: AmneziaStyle.color.mutedGray

                    buttonTextLabel.lineHeight: 16
                    buttonTextLabel.font.pixelSize: 13
                    buttonTextLabel.font.weight: 400

                    text: root.apiAvailableProtocols.length > 1
                        ? root.protocolDisplayName(root.apiCurrentProtocol)
                        : root.protocolDisplayName(root.apiAvailableProtocols[0])
                    leftImageSource: "qrc:/images/controls/arrow-left-right.svg"
                    leftImageColor: AmneziaStyle.color.mutedGray

                    rightImageSource: enabled ? "qrc:/images/controls/chevron-down.svg" : ""

                    Keys.onEnterPressed: this.clicked()
                    Keys.onReturnPressed: this.clicked()

                    onClicked: {
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
                }
            }
        }
    }

    Item {
        id: collapsedBar
        objectName: "collapsedBar"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: collapsedSummary.height

        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -border.width

            topLeftRadius: 16
            topRightRadius: 16
            color: AmneziaStyle.color.onyxBlack
            border.color: AmneziaStyle.color.slateGray
            border.width: 1
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

        Loader {
            id: collapsedSummary

            property bool expanded: false
            property bool hovered: collapsedBarMouseArea.containsMouse
            property bool pressed: collapsedBarMouseArea.pressed

            anchors.left: parent.left
            anchors.right: parent.right

            sourceComponent: serverSummaryComponent
        }
    }

    DrawerType2 {
        id: drawer
        objectName: "drawerProtocol"

        bottomY: root.height
        closedY: root.height - collapsedBar.height
        expandedHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : screen.height * 0.77

        Item {
            objectName: "ProtocolDrawerCollapsedContent"

            anchors.fill: parent

            Loader {
                id: expandedSummary

                property bool expanded: true
                property bool hovered: false
                property bool pressed: false

                anchors.left: parent.left
                anchors.right: parent.right

                sourceComponent: serverSummaryComponent
            }

            ColumnLayout {
                id: serversMenuHeader
                objectName: "serversMenuHeader"

                anchors.top: expandedSummary.bottom
                anchors.right: parent.right
                anchors.left: parent.left

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    spacing: 8

                    visible: !ServersUiController.isDefaultServerFromApi

                    DropDownType {
                        id: containersDropDown
                        objectName: "containersDropDown"

                        Component.onCompleted: root.containersDropDownRef = containersDropDown

                        rootButtonImageColor: AmneziaStyle.color.midnightBlack
                        rootButtonBackgroundColor: AmneziaStyle.color.paleGray
                        rootButtonBackgroundHoveredColor: AmneziaStyle.color.mistyGray
                        rootButtonBackgroundPressedColor: AmneziaStyle.color.cloudyGray
                        rootButtonHoveredBorderColor: AmneziaStyle.color.transparent
                        rootButtonDefaultBorderColor: AmneziaStyle.color.transparent
                        rootButtonTextTopMargin: 8
                        rootButtonTextBottomMargin: 8

                        text: ServersUiController.defaultServerDefaultContainerName
                        textColor: AmneziaStyle.color.midnightBlack
                        headerText: qsTr("VPN protocol")
                        headerBackButtonImage: "qrc:/images/controls/arrow-left.svg"

                        rootButtonClickedFunction: function() {
                            containersDropDown.open()
                        }

                        listView: HomeContainersListView {
                            id: containersListView
                            objectName: "containersListView"

                            rootWidth: root.width

                            Connections {
                                objectName: "rowLayoutConnections"

                                target: ServersUiController

                                function onDefaultServerIdChanged() {
                                    updateContainersModelFilters()
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

                Header2Type {
                    Layout.fillWidth: true
                    Layout.topMargin: 48
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    headerText: qsTr("Servers")
                }
            }

            ButtonGroup {
                id: serversRadioButtonGroup
                objectName: "serversRadioButtonGroup"
            }

            ServersListView {
                id: serversMenuContent
                objectName: "serversMenuContent"
            }
        }
    }

    DrawerType2 {
        id: protocolSelectionDrawer
        objectName: "protocolSelectionDrawer"

        expandedHeight: windowHeight * 0.5

        Item {
            id: protocolDrawerContainer
            anchors.fill: parent

            ColumnLayout {
                id: protocolDrawerHeader

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

                BackButtonType {
                    id: protocolDrawerBackButton

                    Layout.fillWidth: true

                    backButtonImage: "qrc:/images/controls/arrow-left.svg"
                    backButtonFunction: function() { protocolSelectionDrawer.close() }
                }

                Header2Type {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    headerText: qsTr("VPN protocol")
                }
            }

            ListViewType {
                id: protocolDrawerListView

                anchors.top: protocolDrawerHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.topMargin: 16

                model: root.apiAvailableProtocols

                ButtonGroup {
                    id: protocolDrawerButtonGroup
                }

                delegate: Item {
                    implicitWidth: protocolDrawerListView.width
                    implicitHeight: protocolDrawerDelegate.implicitHeight

                    ColumnLayout {
                        id: protocolDrawerDelegate

                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        VerticalRadioButton {
                            id: protocolDrawerRadioButton

                            Layout.fillWidth: true

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

                            Keys.onEnterPressed: protocolDrawerRadioButton.clicked()
                            Keys.onReturnPressed: protocolDrawerRadioButton.clicked()
                        }

                        DividerType {
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
