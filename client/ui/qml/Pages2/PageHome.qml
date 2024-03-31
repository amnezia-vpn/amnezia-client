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

        Item {
            id: focusItem
            KeyNavigation.tab: splitTunnelingButton
        }

        ConnectButton {
            id: connectButton
            anchors.centerIn: parent
        }

        BasicButtonType {
            id: splitTunnelingButton
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 34
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

            property bool isSplitTunnelingEnabled: SitesModel.isTunnelingEnabled ||
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

                onClosed: {
                    if (!GC.isMobile()) {
                        focusItem.forceActiveFocus()
                    }
                }

                parent: root
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

        collapsedContent:  ColumnLayout {
            Connections {
                target: drawer
                enabled: !GC.isMobile()
                function onActiveFocusChanged() {
                    if (drawer.activeFocus && !drawer.isOpened) {
                        collapsedButtonChevron.forceActiveFocus()
                    }
                }
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
                        collapsedButtonChevron.backgroundColor = collapsedButtonChevron.hoveredColor
                        collapsedButtonHeader.opacity = 0.8
                    }

                    function onExited() {
                        collapsedButtonChevron.backgroundColor = collapsedButtonChevron.defaultColor
                        collapsedButtonHeader.opacity = 1
                    }

                    function onPressed(pressed, entered) {
                        collapsedButtonChevron.backgroundColor = pressed ? collapsedButtonChevron.pressedColor : entered ? collapsedButtonChevron.hoveredColor : collapsedButtonChevron.defaultColor
                        collapsedButtonHeader.opacity = 0.7
                    }
                }

                Header1TextType {
                    id: collapsedButtonHeader
                    Layout.maximumWidth: drawer.width - 48 - 18 - 12 // todo

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
                    Keys.onTabPressed: lastItemTabClicked(focusItem)


                    onClicked: {
                        if (drawer.isCollapsed) {
                            drawer.open()
                        }
                    }
                }
            }

            LabelTextType {
                id: collapsedServerMenuDescription
                Layout.bottomMargin: 44
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                text: ServersModel.defaultServerDescriptionCollapsed
            }
        }

        expandedContent: Item {
            id: serverMenuContainer

            implicitHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : screen.height * 0.77

            Component.onCompleted: {
                drawer.expandedHeight = serverMenuContainer.implicitHeight
            }

            Connections {
                target: drawer
                enabled: !GC.isMobile()
                function onOpened() {
                    focusItem1.forceActiveFocus()
                }
            }

            ColumnLayout {
                id: serversMenuHeader

                anchors.top: parent.top
                anchors.right: parent.right
                anchors.left: parent.left

                Header1TextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 14
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: ServersModel.defaultServerName
                    horizontalAlignment: Qt.AlignHCenter
                    maximumLineCount: 2
                    elide: Qt.ElideRight
                }

                LabelTextType {
                    id: expandedServersMenuDescription
                    Layout.bottomMargin: ServersModel.isDefaultServerFromApi ? 69 : 24
                    Layout.fillWidth: true
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    text: ServersModel.defaultServerDescriptionExpanded
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    spacing: 8

                    visible: !ServersModel.isDefaultServerFromApi
                    onVisibleChanged: expandedServersMenuDescription.Layout

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

            FlickableType {
                id: serversContainer

                anchors.top: serversMenuHeader.bottom
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.topMargin: 16

                contentHeight: col.height + col.anchors.bottomMargin
                implicitHeight: parent.height - serversMenuHeader.implicitHeight
                clip: true

                ScrollBar.vertical: ScrollBar {
                    id: scrollBar
                    policy: serversContainer.height >= serversContainer.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
                }

                Keys.onUpPressed: scrollBar.decrease()
                Keys.onDownPressed: scrollBar.increase()

                Column {
                    id: col
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottomMargin: 32

                    spacing: 16

                    ButtonGroup {
                        id: serversRadioButtonGroup
                    }

                    ListView {
                        id: serversMenuContent
                        width: parent.width
                        height: serversMenuContent.contentItem.height

                        model: ServersModel

                        KeyNavigation.tab: focusItem1

                        clip: true
                        interactive: false

                        activeFocusOnTab: true
                        focus: true
                        onActiveFocusChanged: {
                            if (activeFocus) {
                                serversMenuContent.currentIndex = 0
                                serversMenuContent.currentItem.forceActiveFocus()
                            }
                        }

                        onCurrentIndexChanged: {
                            if (currentItem) {
                                serversContainer.ensureVisible(currentItem)
                            }
                        }

                        onVisibleChanged: {
                            if (serversMenuContent.visible && !GC.isMobile()) {
                                currentIndex = 0
                            }
                        }

                        delegate: Item {
                            id: menuContentDelegate

                            property variant delegateData: model

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
                                    VerticalRadioButton {
                                        id: serverRadioButton

                                        Layout.fillWidth: true

                                        text: name
                                        descriptionText: serverDescription

                                        checked: index === ServersModel.defaultIndex
                                        checkable: !ConnectionController.isConnected

                                        ButtonGroup.group: serversRadioButtonGroup

                                        onClicked: {
                                            if (ConnectionController.isConnected) {
                                                PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                                                return
                                            }

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
                                            if (index < serversMenuContent.count - 1) {
                                                serversMenuContent.incrementCurrentIndex()
                                                serversMenuContent.currentItem.forceActiveFocus()
                                            } else {
                                                serversContainer.ensureVisible(serversMenuContent.itemAtIndex(0))
                                                serverInfoButton.focus = false
                                                focusItem1.forceActiveFocus()
                                            }
                                        }
                                        Keys.onEnterPressed: serverInfoButton.clicked()
                                        Keys.onReturnPressed: serverInfoButton.clicked()

                                        onClicked: function() {
                                            ServersModel.processedIndex = index
                                            drawer.close()
                                            PageController.goToPage(PageEnum.PageSettingsServerInfo)
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
    }
}
