import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
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

    QtObject {
        id: routeMode
        property int allApps: 0
        property int onlyForwardApps: 1
        property int allExceptApps: 2
    }

    property list<QtObject> routeModesModel: [
        onlyForwardApps,
        allExceptApps
    ]

    QtObject {
        id: onlyForwardApps
        property string name: qsTr("Only the apps from the list should have access via VPN")
        property int type: routeMode.onlyForwardApps
    }
    QtObject {
        id: allExceptApps
        property string name: qsTr("Apps from the list should not have access via VPN")
        property int type: routeMode.allExceptApps
    }

    function getRouteModesModelIndex() {
        var currentRouteMode = AppSplitTunnelingModel.routeMode
        if ((routeMode.onlyForwardApps === currentRouteMode) || (routeMode.allApps === currentRouteMode)) {
            return 0
        } else if (routeMode.allExceptApps === currentRouteMode) {
            return 1
        }
    }

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }

        RowLayout {
            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16

                headerText: qsTr("App split tunneling")

                enabled: root.pageEnabled
            }

            SwitcherType {
                id: switcher

                Layout.fillWidth: true
                Layout.rightMargin: 16

                enabled: root.pageEnabled

                checked: AppSplitTunnelingModel.isTunnelingEnabled
                onToggled: {                    
                    AppSplitTunnelingModel.toggleSplitTunneling(checked)
                    selector.text = root.routeModesModel[getRouteModesModelIndex()].name
                }
            }
        }

        DropDownType {
            id: selector

            Layout.fillWidth: true
            Layout.topMargin: 32
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            drawerHeight: 0.4375
            drawerParent: root

            headerText: qsTr("Mode")

            enabled: Qt.platform.os === "android" && root.pageEnabled

            listView: ListViewWithRadioButtonType {
                rootWidth: root.width

                model: root.routeModesModel

                currentIndex: getRouteModesModelIndex()

                clickedFunction: function() {
                    selector.text = selectedText
                    selector.close()
                    if (AppSplitTunnelingModel.routeMode !== root.routeModesModel[currentIndex].type) {
                        AppSplitTunnelingModel.routeMode = root.routeModesModel[currentIndex].type
                    }
                }

                Component.onCompleted: {
                    if (root.routeModesModel[currentIndex].type === AppSplitTunnelingModel.routeMode) {
                        selector.text = selectedText
                    } else {
                        selector.text = root.routeModesModel[0].name
                    }
                }

                Connections {
                    target: AppSplitTunnelingModel
                    function onRouteModeChanged() {
                        currentIndex = getRouteModesModelIndex()
                    }
                }
            }
        }
    }

    FlickableType {
        anchors.top: header.bottom
        anchors.topMargin: 16
        contentHeight: col.implicitHeight + addAppButton.implicitHeight + addAppButton.anchors.bottomMargin + addAppButton.anchors.topMargin

        enabled: root.pageEnabled

        Column {
            id: col
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                id: apps
                width: parent.width
                height: apps.contentItem.height

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

                clip: true
                interactive: false

                delegate: Item {
                    implicitWidth: apps.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: appPath
                            rightImageSource: "qrc:/images/controls/trash.svg"
                            rightImageColor: AmneziaStyle.color.paleGray

                            clickedFunction: function() {
                                var headerText = qsTr("Remove ") + appPath + "?"
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    AppSplitTunnelingController.removeApp(proxyAppSplitTunnelingModel.mapToSource(index))
                                }
                                var noButtonFunction = function() {
                                }

                                showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                            }
                        }

                        DividerType {}
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: addAppButton
        anchors.bottomMargin: -24
        color: AmneziaStyle.color.midnightBlack
        opacity: 0.8
    }

    RowLayout {
        id: addAppButton

        enabled: root.pageEnabled

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 24
        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.bottomMargin: 24

        TextFieldWithHeaderType {
            id: searchField

            Layout.fillWidth: true

            textFieldPlaceholderText: qsTr("application name")
            buttonImageSource: "qrc:/images/controls/plus.svg"

            Keys.onTabPressed: lastItemTabClicked(focusItem)
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

        anchors.fill: parent
    }
}
