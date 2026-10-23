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
import "../Config"
import "../Components"

PageType {
    id: root
    
    property bool pageEnabled: true

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: addSitePanel.top

        cacheBuffer: 200
        displayMarginBeginning: 40
        displayMarginEnd: 40

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

        header: ColumnLayout {
            width: listView.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true
                Layout.bottomMargin: 16

                title: qsTr("DNS Exceptions")
                description: qsTr("DNS servers listed here will remain accessible when KillSwitch is active")
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

                text: ip
                trailingIconSource: "qrc:/images/controls/trash.svg"

                onClicked: {
                    var headerText = qsTr("Delete ") + ip + "?"
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        AllowedDnsController.removeDns(dnsFilterModel.mapToSource(index))
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
        placeholderText: qsTr("IPv4 address")
        moreButtonAccessibleName: qsTr("Import / Export addresses")

        onAddClicked: function(text) {
            PageController.showBusyIndicator(true)
            AllowedDnsController.addDns(text)
            PageController.showBusyIndicator(false)
        }

        onMoreClicked: {
            moreActionsMenu.openAbove(addSitePanel.moreButton)
        }
    }

    PopupMenuType {
        id: moreActionsMenu

        PopupMenuItemType {
            id: importSitesButton3

            text: qsTr("Add imported addresses to existing ones")

            onClicked: {
                moreActionsMenu.close()
                root.selectImportFile(false)
            }
        }

        PopupMenuItemType {
            id: importSitesButton2

            text: qsTr("Replace address list")

            onClicked: {
                moreActionsMenu.close()
                root.selectImportFile(true)
            }
        }

        PopupMenuItemType {
            id: exportSitesButton

            text: qsTr("Save address list")

            onClicked: {
                moreActionsMenu.close()
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
                    PageController.showBusyIndicator(false)
                }
            }
        }
    }

    function selectImportFile(replaceExistingSites) {
        var fileName = SystemController.getFileName(qsTr("Open address file"),
                                                    qsTr("Address files (*.json)"))
        if (fileName !== "") {
            PageController.showBusyIndicator(true)
            AllowedDnsController.importDns(fileName, replaceExistingSites)
            PageController.showBusyIndicator(false)
        }
    }
}
