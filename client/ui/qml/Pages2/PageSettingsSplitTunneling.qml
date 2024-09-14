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

    property var isServerFromTelegramApi: ServersModel.getDefaultServerData("isServerFromTelegramApi")
    
    property bool pageEnabled

    Component.onCompleted: {
        if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Cannot change split tunneling settings during active connection"))
            root.pageEnabled = false
        } else if (ServersModel.isDefaultServerDefaultContainerHasSplitTunneling) {
            PageController.showNotificationMessage(qsTr("Default server does not support split tunneling function"))
            root.pageEnabled = false
        } else {
            root.pageEnabled = true
        }
    }

    Connections {
        target: SitesController

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
        var currentRouteMode = SitesModel.routeMode
        if ((routeMode.onlyForwardSites === currentRouteMode) || (routeMode.allSites === currentRouteMode)) {
            return 0
        } else if (routeMode.allExceptSites === currentRouteMode) {
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
                enabled: root.pageEnabled

                Layout.fillWidth: true
                Layout.leftMargin: 16

                headerText: qsTr("Split tunneling")
            }

            SwitcherType {
                id: switcher

                enabled: root.pageEnabled

                Layout.fillWidth: true
                Layout.rightMargin: 16

                function onToggledFunc() {
                    SitesModel.toggleSplitTunneling(this.checked)
                    selector.text = root.routeModesModel[getRouteModesModelIndex()].name
                }

                checked: SitesModel.isTunnelingEnabled
                onToggled: { onToggledFunc() }
                Keys.onEnterPressed: { onToggledFunc() }
                Keys.onReturnPressed: { onToggledFunc() }
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

            enabled: root.pageEnabled

            headerText: qsTr("Mode")

            listView: ListViewWithRadioButtonType {
                rootWidth: root.width

                model: root.routeModesModel

                currentIndex: getRouteModesModelIndex()

                clickedFunction: function() {
                    selector.text = selectedText
                    selector.close()
                    if (SitesModel.routeMode !== root.routeModesModel[currentIndex].type) {
                        SitesModel.routeMode = root.routeModesModel[currentIndex].type
                    }
                }

                Component.onCompleted: {
                    if (root.routeModesModel[currentIndex].type === SitesModel.routeMode) {
                        selector.text = selectedText
                    } else {
                        selector.text = root.routeModesModel[0].name
                    }
                }

                Connections {
                    target: SitesModel
                    function onRouteModeChanged() {
                        currentIndex = getRouteModesModelIndex()
                    }
                }
            }

            // KeyNavigation.tab: {
            //     return sites.count > 0 ?
            //                 sites :
            //                 searchField.textField
            // }
        }
    }

    FlickableType {
        id: fl
        anchors.top: header.bottom
        anchors.topMargin: 16
        contentHeight: col.implicitHeight + addSiteButton.implicitHeight + addSiteButton.anchors.bottomMargin + addSiteButton.anchors.topMargin

        enabled: root.pageEnabled

        Column {
            id: col
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                id: sites
                width: parent.width
                height: sites.contentItem.height

                model: SortFilterProxyModel {
                    id: proxySitesModel
                    sourceModel: SitesModel
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

                clip: true
                interactive: false

                activeFocusOnTab: true
                focus: true
                Keys.onTabPressed: {
                    if (currentIndex < this.count - 1) {
                        this.incrementCurrentIndex()
                    } else {
                        currentIndex = 0
                        searchField.textField.forceActiveFocus()
                    }

                    fl.ensureVisible(currentItem)
                }

                delegate: Item {
                    implicitWidth: sites.width
                    implicitHeight: delegateContent.implicitHeight

                    onActiveFocusChanged: {
                        if (activeFocus) {
                            site.rightButton.forceActiveFocus()
                        }
                    }

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LabelWithButtonType {
                            id: site
                            Layout.fillWidth: true

                            text: url
                            descriptionText: ip
                            rightImageSource: "qrc:/images/controls/trash.svg"
                            rightImageColor: AmneziaStyle.color.paleGray

                            clickedFunction: function() {
                                var headerText = qsTr("Remove ") + url + "?"
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    SitesController.removeSite(proxySitesModel.mapToSource(index))
                                    if (!GC.isMobile()) {
                                        site.rightButton.forceActiveFocus()
                                    }
                                }
                                var noButtonFunction = function() {
                                    if (!GC.isMobile()) {
                                        site.rightButton.forceActiveFocus()
                                    }
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
        anchors.fill: addSiteButton
        anchors.bottomMargin: -24
        color: AmneziaStyle.color.midnightBlack
        opacity: 0.8
    }

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

            textFieldPlaceholderText: qsTr("website or IP")
            buttonImageSource: "qrc:/images/controls/plus.svg"
            // KeyNavigation.tab: GC.isMobile() ? focusItem : addSiteButtonImage

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                SitesController.addSite(textFieldText)
                textFieldText = ""
                PageController.showBusyIndicator(false)
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

            Keys.onTabPressed: lastItemTabClicked(focusItem)
        }
    }

    DrawerType2 {
        id: moreActionsDrawer

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        onClosed: {
            // if (root.defaultActiveFocusItem && !GC.isMobile()) {
                // root.defaultActiveFocusItem.forceActiveFocus()
            // }
        }

        expandedStateContent: ColumnLayout {
            id: moreActionsDrawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Connections {
                target: moreActionsDrawer

                function onOpened() {
                    focusItem1.forceActiveFocus()
                }

                function onActiveFocusChanged() {
                    if (!GC.isMobile()) {
                        focusItem1.forceActiveFocus()
                    }
                }
            }

            Header2Type {
                Layout.fillWidth: true
                Layout.margins: 16

                headerText: qsTr("Import / Export Sites")
            }

            LabelWithButtonType {
                id: importSitesButton
                Layout.fillWidth: true

                text: qsTr("Import")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    importSitesDrawer.openTriggered()
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: exportSitesButton
                Layout.fillWidth: true
                text: qsTr("Save site list")

                clickedFunction: function() {
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
                        SitesController.exportSites(fileName)
                        moreActionsDrawer.closeTriggered()
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            DividerType {}
        }
    }

    DrawerType2 {
        id: importSitesDrawer

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        onClosed: {
            if (!GC.isMobile()) {
                moreActionsDrawer.forceActiveFocus()
            }
        }

        expandedStateContent: Item {
            implicitHeight: importSitesDrawer.expandedHeight

            Connections {
                target: importSitesDrawer
                enabled: !GC.isMobile()
                function onOpened() {
                    focusItem2.forceActiveFocus()
                }
            }

            BackButtonType {
                id: importSitesDrawerBackButton

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

                backButtonFunction: function() {
                    importSitesDrawer.closeTriggered()
                }
            }

            FlickableType {
                anchors.top: importSitesDrawerBackButton.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                contentHeight: importSitesDrawerContent.height

                ColumnLayout {
                    id: importSitesDrawerContent

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right

                    Header2Type {
                        Layout.fillWidth: true
                        Layout.margins: 16

                        headerText: qsTr("Import a list of sites")
                    }

                    LabelWithButtonType {
                        id: importSitesButton2
                        Layout.fillWidth: true

                        text: qsTr("Replace site list")

                        clickedFunction: function() {
                            var fileName = SystemController.getFileName(qsTr("Open sites file"),
                                                                        qsTr("Sites files (*.json)"))
                            if (fileName !== "") {
                                importSitesDrawerContent.importSites(fileName, true)
                            }
                        }
                    }

                    DividerType {}

                    LabelWithButtonType {
                        id: importSitesButton3
                        Layout.fillWidth: true
                        text: qsTr("Add imported sites to existing ones")

                        clickedFunction: function() {
                            var fileName = SystemController.getFileName(qsTr("Open sites file"),
                                                                        qsTr("Sites files (*.json)"))
                            if (fileName !== "") {
                                importSitesDrawerContent.importSites(fileName, false)
                            }
                        }
                    }

                    function importSites(fileName, replaceExistingSites) {
                        PageController.showBusyIndicator(true)
                        SitesController.importSites(fileName, replaceExistingSites)
                        PageController.showBusyIndicator(false)
                        importSitesDrawer.closeTriggered()
                        moreActionsDrawer.closeTriggered()
                    }

                    DividerType {}
                }
            }
        }
    }
}
