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

    Connections {
        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            buttonContent.state = "expanded"
            if (isContainerInstalled) {
                containersDropDown.rootButtonClickedFunction()
            }
        }
        function onForceCloseDrawer() {
            buttonContent.state = "collapsed"
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: buttonContent.state === "expanded"
        onClicked: {
            buttonContent.state = "collapsed"
        }
    }

    Item {
        anchors.fill: parent
        anchors.bottomMargin: buttonContent.collapsedHeight

        ConnectButton {
            anchors.centerIn: parent
        }
    }

    MouseArea {
        id: dragArea

        anchors.fill: buttonBackground
        cursorShape: buttonContent.state === "collapsed" ? Qt.PointingHandCursor : Qt.ArrowCursor
        hoverEnabled: true

        drag.target: buttonContent
        drag.axis: Drag.YAxis
        drag.maximumY: root.height - buttonContent.collapsedHeight
        drag.minimumY: root.height - root.height * 0.9

        /** If drag area is released at any point other than min or max y, transition to the other state */
        onReleased: {
            if (buttonContent.state === "collapsed" && buttonContent.y < dragArea.drag.maximumY) {
                buttonContent.state = "expanded"
                return
            }
            if (buttonContent.state === "expanded" && buttonContent.y > dragArea.drag.minimumY) {
                buttonContent.state = "collapsed"
                return
            }
        }

        onEntered: {
            collapsedButtonChevron.backgroundColor = collapsedButtonChevron.hoveredColor
            collapsedButtonHeader.opacity = 0.8
        }
        onExited: {
            collapsedButtonChevron.backgroundColor = collapsedButtonChevron.defaultColor
            collapsedButtonHeader.opacity = 1
        }
        onPressedChanged: {
            collapsedButtonChevron.backgroundColor = pressed ? collapsedButtonChevron.pressedColor : entered ? collapsedButtonChevron.hoveredColor : collapsedButtonChevron.defaultColor
            collapsedButtonHeader.opacity = 0.7
        }


        onClicked: {
            if (buttonContent.state === "collapsed") {
                buttonContent.state = "expanded"
            }
        }
    }

    Rectangle {
        id: buttonBackground

        anchors { left: buttonContent.left; right: buttonContent.right; top: buttonContent.top }
        height: root.height
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

    ColumnLayout {
        id: buttonContent

        /** Initial height of button content */
        property int collapsedHeight: 0
        /** True when expanded objects should be visible */
        property bool expandedVisibility: buttonContent.state === "expanded" || (buttonContent.state === "collapsed" && dragArea.drag.active === true)
        /** True when collapsed objects should be visible */
        property bool collapsedVisibility: buttonContent.state === "collapsed" && dragArea.drag.active === false

        Drag.active: dragArea.drag.active
        anchors.right: root.right
        anchors.left: root.left
        y: root.height - buttonContent.height

        Component.onCompleted: {
            buttonContent.state = "collapsed"
        }

        /** Set once based on first implicit height change once all children are layed out */
        onImplicitHeightChanged: {
            if (buttonContent.state === "collapsed" && collapsedHeight == 0) {
                collapsedHeight = implicitHeight
            }
        }

        onStateChanged: {
            if (buttonContent.state === "collapsed") {
                var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
                if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(initialPageNavigationBarColor)
                }
                PageController.drawerClose()
                return
            }
            if (buttonContent.state === "expanded") {
                if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(0xFF1C1D21)
                }
                PageController.drawerOpen()
                return
            }
        }

        /** Two states of buttonContent, great place to add any future animations for the drawer */
        states: [
            State {
                name: "collapsed"
                PropertyChanges {
                    target: buttonContent
                    y: root.height - collapsedHeight
                }
            },
            State {
                name: "expanded"
                PropertyChanges {
                    target: buttonContent
                    y: dragArea.drag.minimumY

                }
            }
        ]

        transitions: [
            Transition {
                from: "collapsed"
                to: "expanded"
                PropertyAnimation {
                    target: buttonContent
                    properties: "y"
                    duration: 200
                }
            },
            Transition {
                from: "expanded"
                to: "collapsed"
                PropertyAnimation {
                    target: buttonContent
                    properties: "y"
                    duration: 200
                }
            }
        ]

        DividerType {
            Layout.topMargin: 10
            Layout.fillWidth: false
            Layout.preferredWidth: 20
            Layout.preferredHeight: 2
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            visible: (buttonContent.collapsedVisibility || buttonContent.expandedVisibility)
        }

        RowLayout {
            Layout.topMargin: 14
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            visible: buttonContent.collapsedVisibility

            spacing: 0

            Header1TextType {
                id: collapsedButtonHeader
                Layout.maximumWidth: buttonContent.width - 48 - 18 - 12 // todo

                maximumLineCount: 2
                elide: Qt.ElideRight

                text: ServersModel.defaultServerName
                horizontalAlignment: Qt.AlignHCenter

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

                onClicked: {
                    if (buttonContent.state === "collapsed") {
                        buttonContent.state = "expanded"
                    }
                }
            }
        }

        LabelTextType {
            id: collapsedServerMenuDescription
            Layout.bottomMargin: 44
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            visible: buttonContent.collapsedVisibility
            text: ServersModel.defaultServerDescriptionCollapsed
        }

        ColumnLayout {
            id: serversMenuHeader

            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            Layout.fillWidth: true
            visible: buttonContent.expandedVisibility

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
                Layout.bottomMargin: 24
                Layout.fillWidth: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                text: ServersModel.defaultServerDescriptionExpanded
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 8

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

                    text: ServersModel.defaultContainerName
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
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.topMargin: 16
            contentHeight: col.implicitHeight
            implicitHeight: root.height - (root.height * 0.1) - serversMenuHeader.implicitHeight - 52 //todo 52 is tabbar height
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
                                                    && ServersModel.isAmneziaDnsContainerInstalled(index)) {
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
                                    checkable: !ConnectionController.isConnected

                                    ButtonGroup.group: serversRadioButtonGroup

                                    onClicked: {
                                        if (ConnectionController.isConnected) {
                                            PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                                            return
                                        }

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
                                        buttonContent.state = "collapsed"
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
