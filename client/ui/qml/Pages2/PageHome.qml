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

        ConnectButton {
            id: connectButton
            anchors.centerIn: parent
        }

        BasicButtonType {
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

            onClicked: {
                homeSplitTunnelingDrawer.open()
            }

            HomeSplitTunnelingDrawer {
                id: homeSplitTunnelingDrawer

                parent: root
            }
        }
    }


    DrawerType2 {
        id: drawer
        anchors.fill: parent

        collapsedContent:  ColumnLayout {
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
        expandedContent:  Item {
            id: serverMenuContainer

            implicitHeight: root.height * 0.9

            Component.onCompleted: {
                drawer.expandedHeight = serverMenuContainer.implicitHeight
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

                        text: ServersModel.defaultServerDefaultContainerName
                        textColor: "#0E0E11"
                        headerText: qsTr("VPN protocol")
                        headerBackButtonImage: "qrc:/images/controls/arrow-left.svg"

                        rootButtonClickedFunction: function() {
                            containersDropDown.open()
                        }

                        drawerParent: root

                        listView: HomeContainersListView {
                            rootWidth: root.width

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

            Flickable {
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
                        currentIndex: ServersModel.defaultIndex

                        Connections {
                            target: ServersModel
                            function onDefaultServerIndexChanged(serverIndex) {
                                serversMenuContent.currentIndex = serverIndex
                            }
                        }

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
                                    }

                                    ImageButtonType {
                                        image: "qrc:/images/controls/settings.svg"
                                        imageColor: "#D7D8DB"

                                        implicitWidth: 56
                                        implicitHeight: 56

                                        z: 1

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
    }
}
