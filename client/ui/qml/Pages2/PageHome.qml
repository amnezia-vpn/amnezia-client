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

    property string defaultColor: "#1C1D21"

    property string borderColor: "#2C2D30"

    property string defaultServerName: ServersModel.defaultServerName
    property string defaultServerHostName: ServersModel.defaultServerHostName
    property string defaultContainerName: ContainersModel.defaultContainerName

    Connections {
        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            buttonContent.collapse()
            if (isContainerInstalled) {
                containersDropDown.menuVisible = true
            }
        }
        function onForceCloseDrawer() {
            buttonContent.collapse()
        }
    }

    Connections {
        target: ServersModel

        function onDefaultServerIndexChanged() {
            updateDescriptions()
        }
    }

    Connections {
        target: ContainersModel

        function onDefaultContainerChanged() {
            updateDescriptions()
        }
    }

    function updateDescriptions() {
        var description = ""
        if (ServersModel.isDefaultServerHasWriteAccess()) {
            if (SettingsController.isAmneziaDnsEnabled()
                    && ContainersModel.isAmneziaDnsContainerInstalled(ServersModel.getDefaultServerIndex())) {
                description += "Amnezia DNS | "
            }
        } else {
            if (ServersModel.isDefaultServerConfigContainsAmneziaDns()) {
                description += "Amnezia DNS | "
            }
        }

        collapsedServerMenuDescription.text = description + root.defaultContainerName + " | " + root.defaultServerHostName
        expandedServersMenuDescription.text = description + root.defaultServerHostName
    }

    Component.onCompleted: {
        updateDescriptions()
    }

    Item {
        anchors.fill: parent
        anchors.bottomMargin: buttonContent.collapsedHeight

        ConnectButton {
            anchors.centerIn: parent
        }
    }

    Rectangle {
        id: buttonBackground
        anchors { left: buttonContent.left; right: buttonContent.right; top: buttonContent.top }

        radius: 16
        color: root.defaultColor
        border.color: root.borderColor
        border.width: 1

        Rectangle {
            width: parent.radius
            height: parent.radius
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            color: parent.color
        }
    }

    Drawer2Type {
        id: buttonContent
        visible: true

        fullMouseAreaVisible: false

        /** True when expanded objects should be visible */
        property bool expandedVisibility: buttonContent.expanded() || (buttonContent.collapsed() && buttonContent.dragActive)
        /** True when collapsed objects should be visible */
        property bool collapsedVisibility: buttonContent.collapsed() && !buttonContent.dragActive

        width: parent.width
        height: parent.height
        contentHeight: parent.height * 0.9


        ColumnLayout {
            id: collapsedButtonContent

            parent: buttonContent.contentParent

            visible: buttonContent.collapsedVisibility

            anchors.right: parent.right
            anchors.left: parent.left
            anchors.top: parent.top

            onImplicitHeightChanged: {
                if (buttonContent.collapsed() && buttonContent.collapsedHeight === 0) {
                    buttonContent.collapsedHeight = implicitHeight
                }
            }

            RowLayout {
                Layout.topMargin: 24
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                Header1TextType {
                    id: collapsedButtonHeader
                    Layout.maximumWidth: root.width - 48 - 18 - 12 // todo

                    maximumLineCount: 2
                    elide: Qt.ElideRight

                    text: root.defaultServerName

                    Layout.alignment: Qt.AlignLeft
                }


                ImageButtonType {
                    id: collapsedButtonChevron

                    hoverEnabled: false
                    image: "qrc:/images/controls/chevron-down.svg"
                    imageColor: "#d7d8db"

                    horizontalPadding: 0
                    padding: 0
                    spacing: 0

                    Rectangle {
                        id: rightImageBackground
                        anchors.fill: parent
                        radius: 16
                        color: "transparent"

                        Behavior on color {
                            PropertyAnimation { duration: 200 }
                        }
                    }

                    onClicked: {
                        if (buttonContent.collapsed()) {
                            buttonContent.expand()
                        }
                    }
                }
            }

            LabelTextType {
                id: collapsedServerMenuDescription
                Layout.bottomMargin: 44
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                visible: buttonContent.collapsedVisibility
            }
        }

        Component.onCompleted: {
            buttonContent.collapse()
        }

        ColumnLayout {
            id: serversMenuHeader

            parent: buttonContent.contentParent

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left

            visible: buttonContent.expandedVisibility

            Header1TextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: root.defaultServerName
                horizontalAlignment: Qt.AlignHCenter
                maximumLineCount: 2
                elide: Qt.ElideRight
            }

            LabelTextType {
                id: expandedServersMenuDescription
                Layout.bottomMargin: 24
                Layout.fillWidth: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 8

                DropDownType {
                    id: containersDropDown

                    drawerParent: root

                    rootButtonImageColor: "#0E0E11"
                    rootButtonBackgroundColor: "#D7D8DB"
                    rootButtonBackgroundHoveredColor: Qt.rgba(215, 216, 219, 0.8)
                    rootButtonBackgroundPressedColor: Qt.rgba(215, 216, 219, 0.65)
                    rootButtonHoveredBorderColor: "transparent"
                    rootButtonDefaultBorderColor: "transparent"
                    rootButtonTextTopMargin: 8
                    rootButtonTextBottomMargin: 8

                    text: root.defaultContainerName
                    textColor: "#0E0E11"
                    headerText: qsTr("VPN protocol")
                    headerBackButtonImage: "qrc:/images/controls/arrow-left.svg"

                    rootButtonClickedFunction: function() {
                        ServersModel.currentlyProcessedIndex = serversMenuContent.currentIndex
                        containersDropDown.menuVisible = true
                    }

                    listView: HomeContainersListView {
                        rootWidth: root.width

                        Connections {
                            target: ServersModel

                            function onCurrentlyProcessedServerIndexChanged() {
                                updateContainersModelFilters()
                            }
                        }

                        function updateContainersModelFilters() {
                            if (ServersModel.isCurrentlyProcessedServerHasWriteAccess()) {
                                proxyContainersModel.filters = ContainersModelFilters.getWriteAccessProtocolsListFilters()
                            } else {
                                proxyContainersModel.filters = ContainersModelFilters.getReadAccessProtocolsListFilters()
                            }
                        }

                        model: SortFilterProxyModel {
                            id: proxyContainersModel
                            sourceModel: ContainersModel
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
                visible: buttonContent.expandedVisibility

                headerText: qsTr("Servers")
            }
        }

        Flickable {
            id: serversContainer

            parent: buttonContent.contentParent

            anchors.top: serversMenuHeader.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.topMargin: 16
            contentHeight: col.implicitHeight

            visible: buttonContent.expandedVisibility

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

                spacing: 16

                ButtonGroup {
                    id: serversRadioButtonGroup
                }

                ListView {
                    id: serversMenuContent
                    width: parent.width
                    height: serversMenuContent.contentItem.height

                    model: ServersModel
                    currentIndex: ServersModel.defaultIndex

                    clip: true
                    interactive: false

                    delegate: Item {
                        id: menuContentDelegate

                        property variant delegateData: model

                        implicitWidth: serversMenuContent.width
                        implicitHeight: serverRadioButtonContent.implicitHeight

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
                                    descriptionText: {
                                        var description = ""
                                        if (hasWriteAccess) {
                                            if (SettingsController.isAmneziaDnsEnabled()
                                                    && ContainersModel.isAmneziaDnsContainerInstalled(index)) {
                                                description += "Amnezia DNS | "
                                            }
                                        } else {
                                            if (containsAmneziaDns) {
                                                description += "Amnezia DNS | "
                                            }
                                        }

                                        return description += hostName
                                    }

                                    checked: index === serversMenuContent.currentIndex

                                    ButtonGroup.group: serversRadioButtonGroup

                                    onClicked: {
                                        serversMenuContent.currentIndex = index

                                        ServersModel.currentlyProcessedIndex = index
                                        ServersModel.defaultIndex = index
                                    }

                                    MouseArea {
                                        anchors.fill: serverRadioButton
                                        cursorShape: Qt.PointingHandCursor
                                        enabled: false
                                    }
                                }

                                ImageButtonType {
                                    image: "qrc:/images/controls/settings.svg"
                                    imageColor: "#D7D8DB"

                                    implicitWidth: 56
                                    implicitHeight: 56

                                    z: 1

                                    onClicked: function() {
                                        ServersModel.currentlyProcessedIndex = index
                                        PageController.goToPage(PageEnum.PageSettingsServerInfo)
                                        buttonContent.collapse()
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

        onCollapsedEnter: {
            collapsedButtonChevron.backgroundColor = collapsedButtonChevron.hoveredColor
            collapsedButtonHeader.opacity = 0.8
        }

        onCollapsedExited: {
            collapsedButtonChevron.backgroundColor = collapsedButtonChevron.defaultColor
            collapsedButtonHeader.opacity = 1
        }

        onCollapsedPressChanged: {
            collapsedButtonChevron.backgroundColor = buttonContent.drawerDragArea.pressed ?
                        collapsedButtonChevron.pressedColor : buttonContent.drawerDragArea.entered ?
                            collapsedButtonChevron.hoveredColor : collapsedButtonChevron.defaultColor
            collapsedButtonHeader.opacity = 0.7
        }
    }
}
