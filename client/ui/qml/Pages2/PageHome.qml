import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ContainersModelFilters 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    Connections {
        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            drawer.open()
            if (isContainerInstalled) {
                containersDropDown.rootButtonClickedFunction()
            }
        }
    }

    Item {
        anchors.fill: parent
        anchors.bottomMargin: drawer.collapsedHeight

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 34
            anchors.bottomMargin: 34

            Item {
                id: focusItem
                KeyNavigation.tab: loggingButton.visible ?
                                       loggingButton :
                                       connectButton
            }

            BasicButtonType {
                id: loggingButton
                property bool isLoggingEnabled: SettingsController.isLoggingEnabled

                Layout.alignment: Qt.AlignHCenter

                implicitHeight: 36

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#878B91"
                borderWidth: 0

                visible: isLoggingEnabled ? true : false
                text: qsTr("Logging enabled")

                Keys.onEnterPressed: loggingButton.clicked()
                Keys.onReturnPressed: loggingButton.clicked()

                KeyNavigation.tab: connectButton

                onClicked: {
                    PageController.goToPage(PageEnum.PageSettingsLogging)
                }
            }

            ConnectButton {
                id: connectButton
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
                KeyNavigation.tab: splitTunnelingButton
            }

            BasicButtonType {
                id: splitTunnelingButton

                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                Layout.bottomMargin: 34
                leftPadding: 16
                rightPadding: 16

                implicitHeight: 36

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#878B91"
                leftImageColor: "transparent"
                borderWidth: 0

                buttonTextLabel.lineHeight: 20
                buttonTextLabel.font.pixelSize: 14
                buttonTextLabel.font.weight: 500

                property bool isSplitTunnelingEnabled: SitesModel.isTunnelingEnabled || AppSplitTunnelingModel.isTunnelingEnabled ||
                                                       (ServersModel.isDefaultServerDefaultContainerHasSplitTunneling && ServersModel.getDefaultServerData("isServerFromApi"))

                text: isSplitTunnelingEnabled ? qsTr("Split tunneling enabled") : qsTr("Split tunneling disabled")

                imageSource: isSplitTunnelingEnabled ? "qrc:/images/controls/split-tunneling.svg" : ""
                rightImageSource: "qrc:/images/controls/chevron-down.svg"

                Keys.onEnterPressed: splitTunnelingButton.clicked()
                Keys.onReturnPressed: splitTunnelingButton.clicked()

                KeyNavigation.tab: drawer

                onClicked: {
                    homeSplitTunnelingDrawer.open()
                }

                HomeSplitTunnelingDrawer {
                    id: homeSplitTunnelingDrawer
                    parent: root

                    onClosed: {
                        if (!GC.isMobile()) {
                            focusItem.forceActiveFocus()
                        }
                    }
                }
            }
        }
    }


    DrawerType2 {
        id: drawer
        anchors.fill: parent

        onClosed: {
            if (!GC.isMobile()) {
                focusItem.forceActiveFocus()
            }
        }

        collapsedContent: Item {
            implicitHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : screen.height * 0.77
            Component.onCompleted: {
                drawer.expandedHeight = implicitHeight
            }
            Connections {
                target: drawer
                enabled: !GC.isMobile()
                function onActiveFocusChanged() {
                    if (drawer.activeFocus && !drawer.isOpened) {
                        collapsedButtonChevron.forceActiveFocus()
                    }
                }
            }
            ColumnLayout {
                id: collapsed

                anchors.left: parent.left
                anchors.right: parent.right

                Component.onCompleted: {
                    drawer.collapsedHeight = collapsed.implicitHeight
                }

                DividerType {
                    Layout.topMargin: 10
                    Layout.fillWidth: false
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 2
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                }

                RowLayout {
                    Layout.topMargin: 14
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    spacing: 0

                    Connections {
                        target: drawer
                        function onEntered() {
                            if (drawer.isCollapsed) {
                                collapsedButtonChevron.backgroundColor = collapsedButtonChevron.hoveredColor
                                collapsedButtonHeader.opacity = 0.8
                            } else {
                                collapsedButtonHeader.opacity = 1
                            }
                        }

                        function onExited() {
                            if (drawer.isCollapsed) {
                                collapsedButtonChevron.backgroundColor = collapsedButtonChevron.defaultColor
                                collapsedButtonHeader.opacity = 1
                            } else {
                                collapsedButtonHeader.opacity = 1
                            }
                        }

                        function onPressed(pressed, entered) {
                            if (drawer.isCollapsed) {
                                collapsedButtonChevron.backgroundColor = pressed ? collapsedButtonChevron.pressedColor : entered ? collapsedButtonChevron.hoveredColor : collapsedButtonChevron.defaultColor
                                collapsedButtonHeader.opacity = 0.7
                            } else {
                                collapsedButtonHeader.opacity = 1
                            }
                        }
                    }

                    Header1TextType {
                        id: collapsedButtonHeader
                        Layout.maximumWidth: drawer.width - 48 - 18 - 12

                        maximumLineCount: 2
                        elide: Qt.ElideRight

                        text: ServersModel.defaultServerName
                        horizontalAlignment: Qt.AlignHCenter

                        KeyNavigation.tab: tabBar

                        Behavior on opacity {
                            PropertyAnimation { duration: 200 }
                        }
                    }

                    ImageButtonType {
                        id: collapsedButtonChevron

                        Layout.leftMargin: 8

                        visible: drawer.isCollapsed

                        hoverEnabled: false
                        image: "qrc:/images/controls/chevron-down.svg"
                        imageColor: "#d7d8db"

                        icon.width: 18
                        icon.height: 18
                        backgroundRadius: 16
                        horizontalPadding: 4
                        topPadding: 4
                        bottomPadding: 3

                        Keys.onEnterPressed: collapsedButtonChevron.clicked()
                        Keys.onReturnPressed: collapsedButtonChevron.clicked()
                        Keys.onTabPressed: lastItemTabClicked()


                        onClicked: {
                            if (drawer.isCollapsed) {
                                drawer.open()
                            }
                        }
                    }
                }

                LabelTextType {
                    id: collapsedServerMenuDescription
                    Layout.bottomMargin: drawer.isCollapsed ? 44 : ServersModel.isDefaultServerFromApi ? 89 : 44
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    text: drawer.isCollapsed ? ServersModel.defaultServerDescriptionCollapsed : ServersModel.defaultServerDescriptionExpanded
                }
            }

            Connections {
                target: drawer
                enabled: !GC.isMobile()
                function onIsCollapsedChanged() {
                    if (!drawer.isCollapsed) {
                        focusItem1.forceActiveFocus()
                    }
                }
            }

            ColumnLayout {
                id: serversMenuHeader

                anchors.top: collapsed.bottom
                anchors.right: parent.right
                anchors.left: parent.left

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    spacing: 8

                    visible: !ServersModel.isDefaultServerFromApi

                    Item {
                        id: focusItem1
                        KeyNavigation.tab: containersDropDown
                    }

                    DropDownType {
                        id: containersDropDown

                        rootButtonImageColor: "#0E0E11"
                        rootButtonBackgroundColor: "#D7D8DB"
                        rootButtonBackgroundHoveredColor: Qt.rgba(215, 216, 219, 0.8)
                        rootButtonBackgroundPressedColor: Qt.rgba(215, 216, 219, 0.65)
                        rootButtonHoveredBorderColor: "transparent"
                        rootButtonDefaultBorderColor: "transparent"
                        rootButtonTextTopMargin: 8
                        rootButtonTextBottomMargin: 8

                        text: ServersModel.defaultServerDefaultContainerName
                        textColor: "#0E0E11"
                        headerText: qsTr("VPN protocol")
                        headerBackButtonImage: "qrc:/images/controls/arrow-left.svg"

                        rootButtonClickedFunction: function() {
                            containersDropDown.open()
                        }

                        drawerParent: root
                        KeyNavigation.tab: serversMenuContent

                        listView: HomeContainersListView {
                            id: containersListView
                            rootWidth: root.width
                            onVisibleChanged: {
                                if (containersDropDown.visible && !GC.isMobile()) {
                                    focusItem1.forceActiveFocus()
                                }
                            }

                            Connections {
                                target: ServersModel

                                function onDefaultServerIndexChanged() {
                                    updateContainersModelFilters()
                                }
                            }

                            function updateContainersModelFilters() {
                                if (ServersModel.isDefaultServerHasWriteAccess()) {
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
            }

            ListView {
                id: serversMenuContent

                anchors.top: serversMenuHeader.bottom
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.topMargin: 16

                model: ServersModel
                currentIndex: ServersModel.defaultIndex

                ScrollBar.vertical: ScrollBar {
                    id: scrollBar
                    policy: serversMenuContent.height >= serversMenuContent.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
                }


                activeFocusOnTab: true
                focus: true

                property int focusItemIndex: 0
                onActiveFocusChanged: {
                    if (activeFocus) {
                        serversMenuContent.focusItemIndex = 0
                        serversMenuContent.itemAtIndex(focusItemIndex).forceActiveFocus()
                    }
                }

                onFocusItemIndexChanged: {
                    const focusedElement = serversMenuContent.itemAtIndex(focusItemIndex)
                    if (focusedElement) {
                        if (focusedElement.y + focusedElement.height > serversMenuContent.height) {
                            serversMenuContent.contentY = focusedElement.y + focusedElement.height - serversMenuContent.height
                        } else {
                            serversMenuContent.contentY = 0
                        }
                    }
                }

                Keys.onUpPressed: scrollBar.decrease()
                Keys.onDownPressed: scrollBar.increase()

                Connections {
                    target: drawer
                    enabled: !GC.isMobile()
                    function onIsCollapsedChanged() {
                        if (drawer.isCollapsed) {
                            const item = serversMenuContent.itemAtIndex(serversMenuContent.focusItemIndex)
                            if (item) { item.serverRadioButtonProperty.focus = false }
                        }
                    }
                }

                Connections {
                    target: ServersModel
                    function onDefaultServerIndexChanged(serverIndex) {
                        serversMenuContent.currentIndex = serverIndex
                    }
                }

                clip: true

                delegate: Item {
                    id: menuContentDelegate

                    property variant delegateData: model
                    property VerticalRadioButton serverRadioButtonProperty: serverRadioButton

                    implicitWidth: serversMenuContent.width
                    implicitHeight: serverRadioButtonContent.implicitHeight

                    onActiveFocusChanged: {
                        if (activeFocus) {
                            serverRadioButton.forceActiveFocus()
                        }
                    }

                    ColumnLayout {
                        id: serverRadioButtonContent

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            VerticalRadioButton {
                                id: serverRadioButton

                                Layout.fillWidth: true

                                text: name
                                descriptionText: serverDescription

                                checked: index === serversMenuContent.currentIndex
                                checkable: !ConnectionController.isConnected

                                ButtonGroup.group: serversRadioButtonGroup

                                onClicked: {
                                    if (ConnectionController.isConnected) {
                                        PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                                        return
                                    }

                                    serversMenuContent.currentIndex = index

                                    ServersModel.defaultIndex = index
                                }

                                MouseArea {
                                    anchors.fill: serverRadioButton
                                    cursorShape: Qt.PointingHandCursor
                                    enabled: false
                                }

                                Keys.onTabPressed: serverInfoButton.forceActiveFocus()
                                Keys.onEnterPressed: serverRadioButton.clicked()
                                Keys.onReturnPressed: serverRadioButton.clicked()
                            }

                            ImageButtonType {
                                id: serverInfoButton
                                image: "qrc:/images/controls/settings.svg"
                                imageColor: "#D7D8DB"

                                implicitWidth: 56
                                implicitHeight: 56

                                z: 1

                                Keys.onTabPressed: {
                                    if (serversMenuContent.focusItemIndex < serversMenuContent.count - 1) {
                                        serversMenuContent.focusItemIndex++
                                        serversMenuContent.itemAtIndex(serversMenuContent.focusItemIndex).forceActiveFocus()
                                    } else {
                                        focusItem1.forceActiveFocus()
                                        serversMenuContent.contentY = 0
                                    }
                                }
                                Keys.onEnterPressed: serverInfoButton.clicked()
                                Keys.onReturnPressed: serverInfoButton.clicked()

                                onClicked: function() {
                                    ServersModel.processedIndex = index
                                    PageController.goToPage(PageEnum.PageSettingsServerInfo)
                                    drawer.close()
                                }
                            }
                        }

                        DividerType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 0
                            Layout.rightMargin: 0
                        }
                    }
                }
            }
        }
    }
}
