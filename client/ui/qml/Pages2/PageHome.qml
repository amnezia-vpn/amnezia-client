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

    Item {
        anchors.top: parent.top
        anchors.bottom: buttonBackground.top
        anchors.right: parent.right
        anchors.left: parent.left

        ConnectButton {
            anchors.centerIn: parent
        }
    }

    Connections {
        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            menu.visible = true
            if (isContainerInstalled) {
                containersDropDown.menuVisible = true
            }
        }
    }

    Rectangle {
        id: buttonBackground
        anchors.fill: defaultServerInfo

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

    HomeRootMenuButton {
        id: defaultServerInfo
        height: 130
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        text: root.defaultServerName
        rightImageSource: "qrc:/images/controls/chevron-down.svg"

        defaultContainerName: root.defaultContainerName
        defaultServerHostName: root.defaultServerHostName

        clickedFunction: function() {
             menu.visible = true
        }
    }

    DrawerType {
        id: menu

        interactive: {
            if (stackView && stackView.currentItem) {
                return (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) ? true : false
            } else {
                return false
            }
        }
        dragMargin: buttonBackground.height + 56 // page start tabBar height

        width: parent.width
        height: parent.height * 0.9

        ColumnLayout {
            id: serversMenuHeader
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left

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

                actionButtonImage: "qrc:/images/controls/plus.svg"

                headerText: qsTr("Servers")

                actionButtonFunction: function() {
                    menu.visible = false
                    connectionTypeSelection.visible = true
                }
            }

            ConnectionTypeSelectionDrawer {
                id: connectionTypeSelection
            }
        }

        FlickableType {
            anchors.top: serversMenuHeader.bottom
            anchors.topMargin: 16
            contentHeight: col.implicitHeight

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
                                        menu.visible = false
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
