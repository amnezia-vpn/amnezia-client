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

    property var isServerFromTelegramApi: ServersUiController.isServerFromApi(ServersUiController.defaultServerId)
    
    property bool pageEnabled

    Component.onCompleted: {
        if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Cannot change split tunneling settings during active connection"))
            root.pageEnabled = false
        } else if (ServersUiController.isDefaultServerDefaultContainerHasSplitTunneling) {
            PageController.showNotificationMessage(qsTr("Default server does not support split tunneling function"))
            root.pageEnabled = false
        } else {
            root.pageEnabled = true
        }
    }

    Connections {
        target: IpSplitTunnelingController

        function onFinished(message) {
            PageController.showNotificationMessage(message)
        }

        function onErrorOccurred(errorMessage) {
            PageController.showErrorMessage(errorMessage)
        }
    }

    QtObject {
        id: routeMode
        property int allSites: 0
        property int onlyForwardSites: 1
        property int allExceptSites: 2
    }

    property list<QtObject> routeModesModel: [
        onlyForwardSites,
        allExceptSites
    ]

    QtObject {
        id: onlyForwardSites
        property string name: qsTr("Only the sites listed here will be accessed through the VPN")
        property int type: routeMode.onlyForwardSites
    }
    QtObject {
        id: allExceptSites
        property string name: qsTr("Addresses from the list should not be accessed via VPN")
        property int type: routeMode.allExceptSites
    }

    function getRouteModesModelIndex() {
        var currentRouteMode = IpSplitTunnelingController.routeMode
        if ((routeMode.onlyForwardSites === currentRouteMode) || (routeMode.allSites === currentRouteMode)) {
            return 0
        } else if (routeMode.allExceptSites === currentRouteMode) {
            return 1
        }
    }

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addSitePanel.top

        model: SortFilterProxyModel {
            id: proxyIpSplitTunnelingModel
            sourceModel: IpSplitTunnelingModel
            filters: [
                AnyOf {
                    RegExpFilter {
                        roleName: "url"
                        pattern: ".*" + addSitePanel.textField.text + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                    RegExpFilter {
                        roleName: "ip"
                        pattern: ".*" + addSitePanel.textField.text + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                }
            ]
        }

        header: ColumnLayout {
            width: listView.width

            spacing: 0

            NavigationBarType {
                id: navigationBar

                Layout.fillWidth: true

                title: qsTr("Split tunneling")

                header.showSwitcher: true
                header.switcher.checked: IpSplitTunnelingController.isSplitTunnelingEnabled
                header.switcher.enabled: root.pageEnabled

                Connections {
                    target: navigationBar.header

                    function onSwitcherToggled(checked) {
                        IpSplitTunnelingController.toggleSplitTunneling(checked)
                        selector.text = root.routeModesModel[getRouteModesModelIndex()].name
                    }
                }
            }

            DropDownType {
                id: selector

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                drawerHeight: 0.4375

                enabled: root.pageEnabled

                headerText: qsTr("Mode")

                listView: RadioListType {

                    model: root.routeModesModel

                    selectedIndex: getRouteModesModelIndex()

                    onActivated: {
                        selector.text = selectedText
                        selector.close()
                        if (IpSplitTunnelingController.routeMode !== root.routeModesModel[selectedIndex].type) {
                            IpSplitTunnelingController.routeMode = root.routeModesModel[selectedIndex].type
                        }
                    }

                    Component.onCompleted: {
                        if (root.routeModesModel[selectedIndex].type === IpSplitTunnelingController.routeMode) {
                            selector.text = selectedText
                        } else {
                            selector.text = root.routeModesModel[0].name
                        }
                    }

                    Connections {
                        target: IpSplitTunnelingController
                        function onRouteModeChanged() {
                            selectedIndex = getRouteModesModelIndex()
                        }
                    }
                }
            }
        }

        delegate: Item {
            width: listView.width
            implicitHeight: site.implicitHeight

            enabled: root.pageEnabled

            ListItemType {
                id: site

                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                text: url
                subtitle: ip
                trailingIconSource: "qrc:/images/controls/trash.svg"

                onClicked: {
                    var headerText = qsTr("Remove ") + url + "?"
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        IpSplitTunnelingController.removeSite(proxyIpSplitTunnelingModel.mapToSource(index))
                        if (!GC.isMobile()) {
                            site.forceActiveFocus(Qt.TabFocusReason)
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            site.forceActiveFocus(Qt.TabFocusReason)
                        }
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }

    AddSitePanel {
        id: addSitePanel

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: addSitePanel.textField.activeFocus ? 0 : PageController.imeHeight

        enabled: root.pageEnabled
        placeholderText: qsTr("website or IP")

        onAddClicked: function(text) {
            PageController.showBusyIndicator(true)
            IpSplitTunnelingController.addSite(text)
            PageController.showBusyIndicator(false)
        }

        onMoreClicked: {
            moreActionsDrawer.open()
        }
    }

    SheetType {
        id: moreActionsDrawer

        expandedHeight: moreActionsDrawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: moreActionsDrawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            SectionHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32

                headerText: qsTr("Additional options")
            }

            ListItemType {
                id: importSitesButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Import")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    importSitesDrawer.open()
                }
            }

            ListItemType {
                id: exportSitesButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save site list")

                onClicked: {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = "amnezia_sites.json"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save sites"),
                                                                qsTr("Sites files (*.json)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/amnezia_sites",
                                                                true,
                                                                ".json")
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        IpSplitTunnelingController.exportSites(fileName)
                        moreActionsDrawer.close()
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            ListItemType {
                id: clearSitesButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Clear site list")

                onClicked: {
                    var headerText = qsTr("Clear site list?")
                    var descriptionText = qsTr("All sites will be removed from list.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        IpSplitTunnelingController.removeSites()
                        PageController.showBusyIndicator(false)
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }

    SheetType {
        id: importSitesDrawer

        expandedHeight: importSitesDrawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: importSitesDrawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.topMargin: 2
                Layout.preferredHeight: 48

                IconButtonType {
                    x: 16
                    y: 4

                    size: IconButtonType.Small
                    icon.source: "qrc:/images/controls/arrow-left.svg"
                    Accessible.name: qsTr("Back")

                    onClicked: importSitesDrawer.close()
                }
            }

            SectionHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32

                headerText: qsTr("Import a list of sites")
            }

            Repeater {
                model: importOptions

                delegate: ListItemType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: title

                    onClicked: {
                        clickedHandler()
                    }
                }
            }
        }
    }

    property list<QtObject> importOptions: [
        replaceOption,
        addOption,
    ]

    QtObject {
        id: replaceOption

        readonly property string title: qsTr("Replace site list")
        readonly property var clickedHandler: function() {
            var fileName = SystemController.getFileName(qsTr("Open sites file"),
                                                        qsTr("Sites files (*.json)"))
            if (fileName !== "") {
                root.importSites(fileName, true)
            }
        }
    }

    QtObject {
        id: addOption

        readonly property string title: qsTr("Add imported sites to existing ones")
        readonly property var clickedHandler: function() {
            var fileName = SystemController.getFileName(qsTr("Open sites file"),
                                                        qsTr("Sites files (*.json)"))
            if (fileName !== "") {
                root.importSites(fileName, false)
            }
        }
    }

    function importSites(fileName, replaceExistingSites) {
        PageController.showBusyIndicator(true)
        IpSplitTunnelingController.importSites(fileName, replaceExistingSites)
        PageController.showBusyIndicator(false)
        importSitesDrawer.close()
        moreActionsDrawer.close()
    }
}
