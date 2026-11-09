import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool pageEnabled

    Component.onCompleted: {
        if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Cannot change split tunneling settings during active connection"))
            root.pageEnabled = false
        } else {
            root.pageEnabled = true
        }
    }

    readonly property var routeModes: [
        { type: 1, segmentName: qsTr("Via VPN"), name: qsTr("Only the apps from the list should have access via VPN") },
        { type: 2, segmentName: qsTr("Directly"), name: qsTr("Apps from the list should not have access via VPN") }
    ]

    readonly property int routeModeIndex: AppSplitTunnelingController.routeMode === 2 ? 1 : 0

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addAppPanel.top

        model: SortFilterProxyModel {
            id: proxyAppSplitTunnelingModel
            sourceModel: AppSplitTunnelingModel
            filters: RegExpFilter {
                roleName: "appPath"
                pattern: ".*" + searchField.textField.text + ".*"
                caseSensitivity: Qt.CaseInsensitive
            }
            sorters: [
                RoleSorter { roleName: "appPath"; sortOrder: Qt.AscendingOrder }
            ]
        }

        header: ColumnLayout {
            width: listView.width

            z: 2

            spacing: 0

            onHeightChanged: {
                if (!listView.moving) {
                    Qt.callLater(listView.positionViewAtBeginning)
                }
            }

            NavigationBarType {
                id: navigationBar

                Layout.fillWidth: true

                title: qsTr("App split tunneling")
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: root.pageEnabled

                text: qsTr("App split tunneling")

                onClicked: {
                    splitTunnelingSwitch.toggle()
                    splitTunnelingSwitch.toggled()
                }

                SwitchType {
                    id: splitTunnelingSwitch

                    focusPolicy: Qt.NoFocus

                    checked: AppSplitTunnelingController.isSplitTunnelingEnabled

                    onToggled: AppSplitTunnelingController.toggleSplitTunneling(checked)
                }
            }

            SegmentedControlType {
                id: modeSelector

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: (Qt.platform.os === "android") && root.pageEnabled

                model: root.routeModes.map(function(mode) { return mode.segmentName })
                currentIndex: root.routeModeIndex

                onActivated: function(index) {
                    if (AppSplitTunnelingController.routeMode !== root.routeModes[index].type) {
                        AppSplitTunnelingController.routeMode = root.routeModes[index].type
                    }
                }

                Connections {
                    target: AppSplitTunnelingController

                    function onRouteModeChanged() {
                        modeSelector.currentIndex = root.routeModeIndex
                    }
                }
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                text: root.routeModes[modeSelector.currentIndex].name
                color: AmneziaStyle.color.textSecondary
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                text: qsTr("Only \"Apps from the list should not have access via VPN\" mode is available on Windows")

                visible: (Qt.platform.os === "windows") && root.pageEnabled
            }
        }

        delegate: Item {
            width: listView.width
            implicitHeight: appItem.implicitHeight

            ListItemType {
                id: appItem

                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                text: appPath
                trailingIconSource: "qrc:/images/controls/trash.svg"

                onClicked: {
                    var headerText = qsTr("Remove ") + appPath + "?"
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        AppSplitTunnelingController.removeApp(proxyAppSplitTunnelingModel.mapToSource(index))
                    }
                    var noButtonFunction = function() {
                    }

                    Dialogs.askQuestion(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }

    Rectangle {
        id: addAppPanel

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchField.textField.activeFocus ? 0 : PageController.imeHeight

        implicitHeight: searchField.implicitHeight + 32 + PageController.safeAreaBottomMargin

        color: AmneziaStyle.color.bgBase

        TextFieldType {
            id: searchField

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            enabled: root.pageEnabled

            textField.placeholderText: qsTr("application name")
            buttonImageSource: "qrc:/images/controls/plus.svg"
            buttonJoined: true

            rightButtonClickedOnEnter: true

            clickedFunc: function() {
                searchField.focus = false
                PageController.showBusyIndicator(true)

                if (Qt.platform.os === "windows") {
                    var fileName = SystemController.getFileName(qsTr("Open executable file"),
                                                                qsTr("Executable files (*.*)"))
                    if (fileName !== "") {
                        AppSplitTunnelingController.addApp(fileName)
                    }
                } else if (Qt.platform.os === "android"){
                    installedAppDrawer.open()
                }

                PageController.showBusyIndicator(false)
            }
        }
    }

    InstalledAppsDrawer {
        id: installedAppDrawer
    }
}
