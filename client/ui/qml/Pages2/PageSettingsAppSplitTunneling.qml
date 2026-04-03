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

    property bool hasVipAccess: FBLinkController.canUseAppSplitTunneling
    property bool pageEnabled

    Component.onCompleted: {
        if (!root.hasVipAccess) {
            PageController.showNotificationMessage(qsTr("Раздельное туннелирование по приложениям доступно только в VIP"))
            root.pageEnabled = false
        } else if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Нельзя менять настройки раздельного туннелирования во время активного подключения"))
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

        readonly property string name: qsTr("Только приложения из списка будут работать через VPN")
        readonly property int type: routeMode.onlyForwardApps
    }

    QtObject {
        id: allExceptApps
        
        readonly property string name: qsTr("Приложения из списка будут открываться без VPN")
        readonly property int type: routeMode.allExceptApps
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

        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        BackButtonType {
            id: backButton
        }

        HeaderTypeWithSwitcher {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: qsTr("Раздельное туннелирование приложений")

            enabled: root.pageEnabled
            showSwitcher: true
            switcher {
                checked: AppSplitTunnelingModel.isTunnelingEnabled
                enabled: root.pageEnabled
            }
            switcherFunction: function(checked) {
                AppSplitTunnelingModel.toggleSplitTunneling(checked)
                selector.text = root.routeModesModel[getRouteModesModelIndex()].name
            }
        }

        WarningType {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            visible: !root.hasVipAccess
            textString: qsTr("Эта настройка доступна только в VIP. Список приложений сохранится, но не будет применяться без VIP-подписки.")
            iconPath: "qrc:/images/controls/alert-circle.svg"
        }

        BasicButtonType {
            Layout.fillWidth: true
            Layout.topMargin: !root.hasVipAccess ? 12 : 0
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            visible: !root.hasVipAccess
            text: qsTr("Открыть VIP-подписку")
            defaultColor: "#EAB308"
            hoveredColor: "#FACC15"
            pressedColor: "#CA8A04"
            textColor: "#FFFFFF"

            clickedFunc: function() {
                PageController.goToPage(PageEnum.PageFBLinkSubscription)
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

            headerText: qsTr("Режим")

            enabled: (Qt.platform.os === "android") && root.pageEnabled

            listView: ListViewWithRadioButtonType {
                rootWidth: root.width

                model: root.routeModesModel

                selectedIndex: getRouteModesModelIndex()

                clickedFunction: function() {
                    selector.text = selectedText
                    selector.closeTriggered()
                    if (AppSplitTunnelingModel.routeMode !== root.routeModesModel[selectedIndex].type) {
                        AppSplitTunnelingModel.routeMode = root.routeModesModel[selectedIndex].type
                    }
                }

                Component.onCompleted: {
                    if (root.routeModesModel[selectedIndex].type === AppSplitTunnelingModel.routeMode) {
                        selector.text = selectedText
                    } else {
                        selector.text = root.routeModesModel[0].name
                    }
                }

                Connections {
                    target: AppSplitTunnelingModel
                    function onRouteModeChanged() {
                        selectedIndex = getRouteModesModelIndex()
                    }
                }
            }
        }

        WarningType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            textString: qsTr("На Windows доступен только режим «Приложения из списка будут открываться без VPN»")
            iconPath: "qrc:/images/controls/alert-circle.svg"

            visible: (Qt.platform.os === "windows") && root.pageEnabled
        }
    }

    ListViewType {
        id: listView

        ScrollBar.vertical: ScrollBarType { policy: ScrollBar.AlwaysOn }

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: addAppButton.implicitHeight + 48 + SettingsController.safeAreaBottomMargin + (searchField.textField.activeFocus ? 0 : SettingsController.imeHeight)
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true

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

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true

                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: appPath
                rightImageSource: "qrc:/images/controls/trash.svg"
                rightImageColor: FBLinkStyle.color.paleGray

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

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        
        height: addAppButton.implicitHeight + 48 + SettingsController.safeAreaBottomMargin
        
        color: FBLinkStyle.color.midnightBlack
        
        RowLayout {
            id: addAppButton

            enabled: root.pageEnabled

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.rightMargin: 16
            anchors.leftMargin: 16
            anchors.bottomMargin: 24 + SettingsController.safeAreaBottomMargin

            TextFieldWithHeaderType {
                id: searchField

                Layout.fillWidth: true

                textField.placeholderText: qsTr("название приложения")
                buttonImageSource: "qrc:/images/controls/plus.svg"

                rightButtonClickedOnEnter: true

                clickedFunc: function() {
                    searchField.focus = false

                    if (Qt.platform.os === "windows") {
                        var fileName = SystemController.getFileName(qsTr("Открыть исполняемый файл"),
                                                                    qsTr("Executable files (*.*)"))
                        if (fileName !== "") {
                            PageController.showBusyIndicator(true)
                            AppSplitTunnelingController.addApp(fileName)
                        }
                    } else if (Qt.platform.os === "android"){
                        installedAppDrawer.openTriggered()
                    }
                }
            }
        }
    }

    InstalledAppsDrawer {
        id: installedAppDrawer

        anchors.fill: parent
    }

    Connections {
        target: AppSplitTunnelingController

        function onFinished(message) {
            PageController.showBusyIndicator(false)
            if (message !== "") {
                PageController.showNotificationMessage(message)
            }
        }

        function onErrorOccurred(errorMessage) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(errorMessage)
        }
    }
}
