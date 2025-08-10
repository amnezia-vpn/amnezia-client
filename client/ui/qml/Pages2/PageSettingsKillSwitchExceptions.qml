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
    
    property bool pageEnabled: true

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }

        BaseHeaderType {
            enabled: root.pageEnabled

            Layout.fillWidth: true
            Layout.leftMargin: 16

            headerText: qsTr("DNS Exceptions")
            descriptionText: qsTr("DNS servers listed here will remain accessible when KillSwitch is active")
        }
    }

    ListView {
        id: listView

        anchors.top: header.bottom
        anchors.topMargin: 16
        anchors.bottom: parent.bottom

        width: parent.width

        enabled: root.pageEnabled

        property bool isFocusable: true

        cacheBuffer: 200
        displayMarginBeginning: 40
        displayMarginEnd: 40
        
        ScrollBar.vertical: ScrollBarType {}
        
        footer: Item {
            width: listView.width
            height: addSitePanel.height
        }
        
        footerPositioning: ListView.InlineFooter

        model: SortFilterProxyModel {
            id: dnsFilterModel
            sourceModel: AllowedDnsModel
            filters: [
                RegExpFilter {
                    roleName: "ip"
                    pattern: ".*" + addSitePanel.textField.text + ".*"
                    caseSensitivity: Qt.CaseInsensitive
                }
            ]
        }

        clip: true

        reuseItems: true

        delegate: ColumnLayout {
            id: delegateContent

            width: listView.width

            LabelWithButtonType {
                id: site
                Layout.fillWidth: true

                text: ip
                rightImageSource: "qrc:/images/controls/trash.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    var headerText = qsTr("Delete ") + ip + "?"
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        AllowedDnsController.removeDns(dnsFilterModel.mapToSource(index))
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

    AddSitePanel {
        id: addSitePanel
        
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        
        enabled: root.pageEnabled
        placeholderText: qsTr("IPv4 address")

        onAddClicked: function(text) {
            PageController.showBusyIndicator(true)
            AllowedDnsController.addDns(text)
            PageController.showBusyIndicator(false)
        }

        onMoreClicked: {
            moreActionsDrawer.openTriggered()
        }
    }

    DrawerType2 {
        id: moreActionsDrawer

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        expandedStateContent: ColumnLayout {
            id: moreActionsDrawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Header2Type {
                Layout.fillWidth: true
                Layout.margins: 16

                headerText: qsTr("Import / Export addresses")
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
                text: qsTr("Save address list")

                clickedFunction: function() {
                    var fileName = ""
                    if (GC.isMobile()) {
                        fileName = "amnezia_killswitch_exceptions.json"
                    } else {
                        fileName = SystemController.getFileName(qsTr("Save addresses"),
                                                                qsTr("Address files (*.json)"),
                                                                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/amnezia_killswitch_exceptions",
                                                                true,
                                                                ".json")
                    }
                    if (fileName !== "") {
                        PageController.showBusyIndicator(true)
                        AllowedDnsController.exportDns(fileName)
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

                        headerText: qsTr("Import address list")
                    }

                    LabelWithButtonType {
                        id: importSitesButton2
                        Layout.fillWidth: true

                        text: qsTr("Replace address list")

                        clickedFunction: function() {
                            var fileName = SystemController.getFileName(qsTr("Open address file"),
                                                                        qsTr("Address files (*.json)"))
                            if (fileName !== "") {
                                importSitesDrawerContent.importSites(fileName, true)
                            }
                        }
                    }

                    DividerType {}

                    LabelWithButtonType {
                        id: importSitesButton3
                        Layout.fillWidth: true
                        text: qsTr("Add imported addresses to existing ones")

                        clickedFunction: function() {
                            var fileName = SystemController.getFileName(qsTr("Open address file"),
                                                                        qsTr("Address files (*.json)"))
                            if (fileName !== "") {
                                importSitesDrawerContent.importSites(fileName, false)
                            }
                        }
                    }

                    function importSites(fileName, replaceExistingSites) {
                        PageController.showBusyIndicator(true)
                        AllowedDnsController.importDns(fileName, replaceExistingSites)
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
