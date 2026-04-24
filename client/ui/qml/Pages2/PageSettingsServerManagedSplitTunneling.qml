import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import SortFilterProxyModel 0.2

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool pageEnabled: true

    QtObject {
        id: routeMode
        property int allExceptSites: 2
    }

    function addManagedSite() {
        if (searchField.textField.text === "") {
            return
        }

        PageController.showBusyIndicator(true)
        SitesController.addManagedSite(routeMode.allExceptSites, searchField.textField.text)
        searchField.textField.text = ""
        PageController.showBusyIndicator(false)
    }

    Component.onCompleted: {
        root.pageEnabled = SitesController.canEditManagedSites()
        SitesController.reloadManagedSites()
    }

    Connections {
        target: SitesController

        function onFinished(message) {
            PageController.showNotificationMessage(message)
        }

        function onErrorOccurred(errorMessage) {
            PageController.showErrorMessage(errorMessage)
        }

        function onManagedSplitTunnelingForceChanged() {
            forceSwitcher.checked = SitesController.isManagedSplitTunnelingForceEnabled()
        }
    }

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        BackButtonType {}

        BaseHeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: qsTr("Server routing rules")
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            wrapMode: Text.WordWrap
            color: AmneziaStyle.color.paleGray
            text: qsTr("These rules are stored with the server config and are applied in Amnezia VPN only when this server is active.")
        }

        SwitcherType {
            id: forceSwitcher

            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Force split tunneling")
            descriptionText: qsTr("For clients with split tunneling disabled, enable bypass mode and apply these server bypass rules. Clients using the opposite split tunneling mode keep their own settings.")

            enabled: root.pageEnabled
            checked: SitesController.isManagedSplitTunnelingForceEnabled()

            onToggled: function() {
                SitesController.setManagedSplitTunnelingForceEnabled(checked)
            }
        }
    }

    ListViewType {
        id: listView

        ScrollBar.vertical: ScrollBarType { policy: ScrollBar.AlwaysOn }

        anchors.top: header.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: addSiteButton.implicitHeight + 48 + (searchField.textField.activeFocus ? 0 : PageController.imeHeight)

        width: parent.width
        enabled: root.pageEnabled
        clip: true

        model: SortFilterProxyModel {
            id: proxySitesModel
            sourceModel: ManagedExceptSitesModel
            filters: [
                AnyOf {
                    RegExpFilter {
                        roleName: "url"
                        pattern: ".*" + searchField.textField.text + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                    RegExpFilter {
                        roleName: "ip"
                        pattern: ".*" + searchField.textField.text + ".*"
                        caseSensitivity: Qt.CaseInsensitive
                    }
                }
            ]
        }

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                id: site
                Layout.fillWidth: true

                text: url
                descriptionText: ip
                rightImageSource: root.pageEnabled ? "qrc:/images/controls/trash.svg" : ""
                rightImageColor: AmneziaStyle.color.paleGray
                enabled: root.pageEnabled

                clickedFunction: function() {
                    var yesButtonFunction = function() {
                        SitesController.removeManagedSite(routeMode.allExceptSites, proxySitesModel.mapToSource(index))
                        if (!GC.isMobile()) {
                            site.rightButton.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            site.rightButton.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(qsTr("Remove ") + url + "?", "", qsTr("Continue"), qsTr("Cancel"), yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {}
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: addSiteButton.implicitHeight + 48
        color: AmneziaStyle.color.midnightBlack

        RowLayout {
            id: addSiteButton

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
                rightButtonClickedOnEnter: true
                textField.placeholderText: qsTr("website or IP/subnet")
                buttonImageSource: "qrc:/images/controls/plus.svg"

                clickedFunc: function() {
                    root.addManagedSite()
                }
            }

            ImageButtonType {
                id: addSiteButtonImage
                implicitWidth: 56
                implicitHeight: 56
                image: "qrc:/images/controls/more-vertical.svg"
                imageColor: AmneziaStyle.color.paleGray

                onClicked: function () {
                    moreActionsDrawer.openTriggered()
                }

                Keys.onReturnPressed: addSiteButtonImage.clicked()
                Keys.onEnterPressed: addSiteButtonImage.clicked()
            }
        }
    }

    DrawerType2 {
        id: moreActionsDrawer

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        expandedStateContent: ColumnLayout {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Header2Type {
                Layout.fillWidth: true
                Layout.margins: 16
                headerText: qsTr("Additional options")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Import")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                enabled: root.pageEnabled

                clickedFunction: function() {
                    importSitesDrawer.openTriggered()
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Save site list")
                enabled: root.pageEnabled

                clickedFunction: function() {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = "amnezia_server_routing_rules.json"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save sites"),
                                                                qsTr("Sites files (*.json)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/amnezia_server_routing_rules",
                                                                true,
                                                                ".json")
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        SitesController.exportManagedSites(routeMode.allExceptSites, fileName)
                        moreActionsDrawer.closeTriggered()
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Clear site list")
                enabled: root.pageEnabled

                clickedFunction: function() {
                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        SitesController.removeManagedSites(routeMode.allExceptSites)
                        moreActionsDrawer.closeTriggered()
                        PageController.showBusyIndicator(false)
                    }
                    showQuestionDrawer(qsTr("Clear site list?"), qsTr("All sites will be removed from list."),
                                       qsTr("Continue"), qsTr("Cancel"), yesButtonFunction, function() {})
                }
            }

            DividerType {}
        }
    }

    DrawerType2 {
        id: importSitesDrawer

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        expandedStateContent: Item {
            implicitHeight: importSitesDrawer.expandedHeight

            BackButtonType {
                id: importSitesDrawerBackButton

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

                backButtonFunction: function() {
                    importSitesDrawer.closeTriggered()
                }

                onFocusChanged: {
                    if (this.activeFocus) {
                        importSitesDrawerListView.positionViewAtBeginning()
                    }
                }
            }

            ListViewType {
                id: importSitesDrawerListView

                anchors.top: importSitesDrawerBackButton.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                header: ColumnLayout {
                    width: importSitesDrawerListView.width

                    Header2Type {
                        Layout.fillWidth: true
                        Layout.margins: 16
                        headerText: qsTr("Import a list of sites")
                    }
                }

                model: importOptions

                delegate: ColumnLayout {
                    width: importSitesDrawerListView.width

                    LabelWithButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        text: title

                        clickedFunction: function() {
                            clickedHandler()
                        }
                    }

                    DividerType {}
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
            var fileName = SystemController.getFileName(qsTr("Open sites file"), qsTr("Sites files (*.json)"))
            if (fileName !== "") {
                root.importManagedSites(fileName, true)
            }
        }
    }

    QtObject {
        id: addOption
        readonly property string title: qsTr("Add imported sites to existing ones")
        readonly property var clickedHandler: function() {
            var fileName = SystemController.getFileName(qsTr("Open sites file"), qsTr("Sites files (*.json)"))
            if (fileName !== "") {
                root.importManagedSites(fileName, false)
            }
        }
    }

    function importManagedSites(fileName, replaceExistingSites) {
        PageController.showBusyIndicator(true)
        SitesController.importManagedSites(routeMode.allExceptSites, fileName, replaceExistingSites)
        PageController.showBusyIndicator(false)
        importSitesDrawer.closeTriggered()
        moreActionsDrawer.closeTriggered()
    }
}
