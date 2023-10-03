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

    Connections {
        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            buttonContent.state = "expanded"
            if (isContainerInstalled) {
                containersDropDown.menuVisible = true
            }
        }
        function onForceCloseDrawer() {
            buttonContent.state = "collapsed"
        }
    }

    MouseArea {
        id: dragArea

        anchors.fill: buttonBackground
        cursorShape: Qt.PointingHandCursor
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

        RowLayout {
            Layout.topMargin: 24
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            visible: buttonContent.collapsedVisibility

            spacing: 0

            Header1TextType {
                Layout.maximumWidth: buttonContent.width - 48 - 18 - 12 // todo

                maximumLineCount: 2
                elide: Qt.ElideRight

                text: root.defaultServerName
                horizontalAlignment: Qt.AlignHCenter
            }

            Image {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18

                Layout.leftMargin: 12

                source: "qrc:/images/controls/chevron-down.svg"
            }
        }

        LabelTextType {
            Layout.bottomMargin: 44
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            visible: buttonContent.collapsedVisibility

            text: {
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

                description += root.defaultContainerName + " | " + root.defaultServerHostName
                return description
            }
        }

        ColumnLayout {
            id: serversMenuHeader

            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            Layout.fillWidth: true
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
                Layout.bottomMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                text: root.defaultServerHostName
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 8

                DropDownType {
                    id: containersDropDown

                    rootButtonImageColor: "#0E0E11"
                    rootButtonBackgroundColor: "#D7D8DB"
                    rootButtonHoveredBorderColor: "transparent"
                    rootButtonPressedBorderColor: "transparent"
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

                actionButtonImage: "qrc:/images/controls/plus.svg"

                headerText: qsTr("Servers")

                actionButtonFunction: function() {
                    buttonContent.state = "collapsed"
                    connectionTypeSelection.visible = true
                }
            }

            ConnectionTypeSelectionDrawer {
                id: connectionTypeSelection
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
                                                    && ContainersModel.isAmneziaDnsContainerInstalled(index)) {
                                                description += "AmneziaDNS | "
                                            }
                                        } else {
                                            if (containsAmneziaDns) {
                                                description += "AmneziaDNS | "
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
                                        buttonContent.state = "collapsed"
                                    }
                                }
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
}
