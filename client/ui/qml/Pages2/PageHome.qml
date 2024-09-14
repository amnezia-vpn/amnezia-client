import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
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

    Connections {
        objectName: "pageControllerConnections"

        target: PageController

        function onRestorePageHomeState(isContainerInstalled) {
            drawer.openTriggered()
            if (isContainerInstalled) {
                containersDropDown.rootButtonClickedFunction()
            }
        }
    }

    Item {
        objectName: "homeColumnItem"

        anchors.fill: parent
        anchors.bottomMargin: drawer.collapsedHeight

        ColumnLayout {
            objectName: "homeColumnLayout"

            anchors.fill: parent
            anchors.topMargin: 34
            anchors.bottomMargin: 34

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

                Keys.onEnterPressed: loggingButton.clicked()
                Keys.onReturnPressed: loggingButton.clicked()

                onClicked: {
                    PageController.goToPage(PageEnum.PageSettingsLogging)
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
                Layout.bottomMargin: 34
                leftPadding: 16
                rightPadding: 16

                implicitHeight: 36

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.mutedGray
                leftImageColor: AmneziaStyle.color.transparent
                borderWidth: 0

                buttonTextLabel.lineHeight: 20
                buttonTextLabel.font.pixelSize: 14
                buttonTextLabel.font.weight: 500

                property bool isSplitTunnelingEnabled: SitesModel.isTunnelingEnabled || AppSplitTunnelingModel.isTunnelingEnabled ||
                                                       ServersModel.isDefaultServerDefaultContainerHasSplitTunneling

                text: isSplitTunnelingEnabled ? qsTr("Split tunneling enabled") : qsTr("Split tunneling disabled")

                imageSource: isSplitTunnelingEnabled ? "qrc:/images/controls/split-tunneling.svg" : ""
                rightImageSource: "qrc:/images/controls/chevron-down.svg"

                Keys.onEnterPressed: splitTunnelingButton.clicked()
                Keys.onReturnPressed: splitTunnelingButton.clicked()

                onClicked: {
                    homeSplitTunnelingDrawer.openTriggered()
                }

                HomeSplitTunnelingDrawer {
                    id: homeSplitTunnelingDrawer
                    objectName: "homeSplitTunnelingDrawer"

                    parent: root

                    onClosed: {
                        console.log(objectName, " was closed...")
                        FocusController.setRootItem(null)
                    }
                }
            }
        }
    }

    DrawerType2 {
        id: drawer
        objectName: "drawerProtocol"

        anchors.fill: parent

        collapsedStateContent: Item {
            objectName: "ProtocolDrawerCollapsedContent"

            implicitHeight: Qt.platform.os !== "ios" ? root.height * 0.9 : screen.height * 0.77
            Component.onCompleted: {
                drawer.expandedHeight = implicitHeight
            }
            Connections {
                objectName: "drawerConnections"

                target: drawer
                enabled: !GC.isMobile()
            }
            ColumnLayout {
                id: collapsed
                objectName: "collapsedColumnLayout"

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
                    objectName: "rowLayout"

                    Layout.topMargin: 14
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    spacing: 0

                    Connections {
                        objectName: "drawerConnections"

                        target: drawer
                        function onCursorEntered() {
                            if (drawer.isCollapsedStateActive) {
                                collapsedButtonChevron.backgroundColor = collapsedButtonChevron.hoveredColor
                                collapsedButtonHeader.opacity = 0.8
                            } else {
                                collapsedButtonHeader.opacity = 1
                            }
                        }

                        function onCursorExited() {
                            if (drawer.isCollapsedStateActive) {
                                collapsedButtonChevron.backgroundColor = collapsedButtonChevron.defaultColor
                                collapsedButtonHeader.opacity = 1
                            } else {
                                collapsedButtonHeader.opacity = 1
                            }
                        }

                        function onPressed(pressed, entered) {
                            if (drawer.isCollapsedStateActive) {
                                collapsedButtonChevron.backgroundColor = pressed ? collapsedButtonChevron.pressedColor : entered ? collapsedButtonChevron.hoveredColor : collapsedButtonChevron.defaultColor
                                collapsedButtonHeader.opacity = 0.7
                            } else {
                                collapsedButtonHeader.opacity = 1
                            }
                        }
                    }

                    Header1TextType {
                        id: collapsedButtonHeader
                        objectName: "collapsedButtonHeader"

                        Layout.maximumWidth: drawer.width - 48 - 18 - 12

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
                        objectName: "collapsedButtonChevron"

                        Layout.leftMargin: 8

                        visible: drawer.isCollapsedStateActive()

                        hoverEnabled: false
                        image: "qrc:/images/controls/chevron-down.svg"
                        imageColor: AmneziaStyle.color.paleGray

                        icon.width: 18
                        icon.height: 18
                        backgroundRadius: 16
                        horizontalPadding: 4
                        topPadding: 4
                        bottomPadding: 3

                        Keys.onEnterPressed: collapsedButtonChevron.clicked()
                        Keys.onReturnPressed: collapsedButtonChevron.clicked()

                        onClicked: {
                            console.debug("onClicked collapsedButtonChevron")
                            if (drawer.isCollapsedStateActive()) {
                                drawer.openTriggered()
                                FocusController.setRootItem(drawer)
                            }
                        }
                    }
                }

                RowLayout {
                    objectName: "rowLayoutLabel"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.bottomMargin: drawer.isCollapsedStateActive ? 44 : ServersModel.isDefaultServerFromApi ? 89 : 44
                    spacing: 0

                    Image {
                        Layout.rightMargin: 8
                        visible: source !== ""
                        source: ServersModel.defaultServerImagePathCollapsed
                    }

                    LabelTextType {
                        id: collapsedServerMenuDescription
                        text: drawer.isCollapsedStateActive ? ServersModel.defaultServerDescriptionCollapsed : ServersModel.defaultServerDescriptionExpanded
                    }
                }
            }

            ColumnLayout {
                id: serversMenuHeader
                objectName: "serversMenuHeader"

                anchors.top: collapsed.bottom
                anchors.right: parent.right
                anchors.left: parent.left

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    spacing: 8

                    visible: !ServersModel.isDefaultServerFromApi

                    DropDownType {
                        id: containersDropDown
                        objectName: "containersDropDown"

                        rootButtonImageColor: AmneziaStyle.color.midnightBlack
                        rootButtonBackgroundColor: AmneziaStyle.color.paleGray
                        rootButtonBackgroundHoveredColor: Qt.rgba(215, 216, 219, 0.8)
                        rootButtonBackgroundPressedColor: Qt.rgba(215, 216, 219, 0.65)
                        rootButtonHoveredBorderColor: AmneziaStyle.color.transparent
                        rootButtonDefaultBorderColor: AmneziaStyle.color.transparent
                        rootButtonTextTopMargin: 8
                        rootButtonTextBottomMargin: 8

                        text: ServersModel.defaultServerDefaultContainerName
                        textColor: AmneziaStyle.color.midnightBlack
                        headerText: qsTr("VPN protocol")
                        headerBackButtonImage: "qrc:/images/controls/arrow-left.svg"

                        rootButtonClickedFunction: function() {
                            containersDropDown.openTriggered()
                        }

                        drawerParent: root

                        listView: HomeContainersListView {
                            id: containersListView
                            objectName: "containersListView"

                            rootWidth: root.width
                            height: 500 // TODO: make calculated

                            // isFocusable: false // TODO: this is a workaround. Need to remove it

                            Connections {
                                objectName: "rowLayoutConnections"

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
                objectName: "serversRadioButtonGroup"
            }

            ServersListView {
                id: serversMenuContent
                objectName: "serversMenuContent"
            }
        }
    }
}
