import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var isServerFromApi: ServersModel.getDefaultServerData("isServerFromApi")
    
    defaultActiveFocusItem: website_ip_field.textField

    property bool pageEnabled: {
        return !ConnectionController.isConnected && !isServerFromApi
    }

    Component.onCompleted: {
        if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Cannot change split tunneling settings during active connection"))
            root.pageEnabled = false
        } else if (ServersModel.isDefaultServerDefaultContainerHasSplitTunneling && isServerFromApi) {
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

                checked: SitesModel.isTunnelingEnabled
                onToggled: {                    
                    SitesModel.toggleSplitTunneling(checked)
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
        }
    }

    FlickableType {
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

                model: SitesModel

                clip: true
                interactive: false

                delegate: Item {
                    implicitWidth: sites.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: url
                            descriptionText: ip
                            rightImageSource: "qrc:/images/controls/trash.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                var headerText = qsTr("Remove ") + url + "?"
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    SitesController.removeSite(index)
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
        anchors.fill: addSiteButton
        anchors.bottomMargin: -24
        color: "#0E0E11"
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
            id: website_ip_field

            Layout.fillWidth: true

            textFieldPlaceholderText: qsTr("website or IP")
            buttonImageSource: "qrc:/images/controls/plus.svg"

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                SitesController.addSite(textFieldText)
                textFieldText = ""
                PageController.showBusyIndicator(false)
            }
        }

        ImageButtonType {
            implicitWidth: 56
            implicitHeight: 56

            image: "qrc:/images/controls/more-vertical.svg"
            imageColor: "#D7D8DB"

            onClicked: function () {
                moreActionsDrawer.open()
            }
        }
    }

    DrawerType2 {
        id: moreActionsDrawer

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        expandedContent: ColumnLayout {
            id: moreActionsDrawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Header2Type {
                Layout.fillWidth: true
                Layout.margins: 16

                headerText: qsTr("Import / Export Sites")
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Import")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    importSitesDrawer.open()
                }
            }

            DividerType {}

            LabelWithButtonType {
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
                        moreActionsDrawer.close()
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

        expandedContent: Item {
            implicitHeight: importSitesDrawer.expandedHeight

            BackButtonType {
                id: importSitesDrawerBackButton

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

                backButtonFunction: function() {
                    importSitesDrawer.close()
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
                        importSitesDrawer.close()
                        moreActionsDrawer.close()
                    }

                    DividerType {}
                }
            }
        }
    }

    QuestionDrawer {
        id: questionDrawer
    }
}
