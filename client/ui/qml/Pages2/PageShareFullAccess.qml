import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 0

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 24

                headerText: qsTr("Full access to the server and VPN")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24

                text: qsTr("We recommend that you use full access to the server only for your own additional devices.\n") +
                      qsTr("If you share full access with other people, they can remove and add protocols and services to the server, which will cause the VPN to work incorrectly for all users. ")
                color: AmneziaStyle.color.mutedGray
            }

            DropDownType {
                id: serverSelector

                signal severSelectorIndexChanged
                property int currentIndex: 0

                Layout.fillWidth: true
                Layout.topMargin: 16

                drawerHeight: 0.4375
                drawerParent: root

                descriptionText: qsTr("Server")
                headerText: qsTr("Server")

                listView: ListViewWithRadioButtonType {
                    id: serverSelectorListView

                    rootWidth: root.width
                    imageSource: "qrc:/images/controls/check.svg"

                    model: SortFilterProxyModel {
                        id: proxyServersModel
                        sourceModel: ServersModel
                        filters: [
                            ValueFilter {
                                roleName: "hasWriteAccess"
                                value: true
                            }
                        ]
                    }

                    clickedFunction: function() {
                        handler()

                        if (serverSelector.currentIndex !== serverSelectorListView.currentIndex) {
                            serverSelector.currentIndex = serverSelectorListView.currentIndex
                        }

                        shareConnectionDrawer.headerText = qsTr("Accessing ") + serverSelector.text
                        shareConnectionDrawer.configContentHeaderText = qsTr("File with accessing settings to ") + serverSelector.text
                        serverSelector.closeTriggered()
                    }

                    Component.onCompleted: {
                        serverSelectorListView.currentIndex = ServersModel.isDefaultServerHasWriteAccess() ?
                                    proxyServersModel.mapFromSource(ServersModel.defaultIndex) : 0
                        serverSelectorListView.triggerCurrentItem()
                    }

                    function handler() {
                        serverSelector.text = selectedText
                        ServersModel.processedIndex = proxyServersModel.mapToSource(currentIndex)
                    }
                }
            }

            BasicButtonType {
                id: shareButton
                Layout.fillWidth: true
                Layout.topMargin: 40

                text: qsTr("Share")
                leftImageSource: "qrc:/images/controls/share-2.svg"

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)

                    if (Qt.platform.os === "android" && !SystemController.isAuthenticated()) {
                        PageController.showBusyIndicator(false)
                        ExportController.exportErrorOccurred(qsTr("Access error!"))
                        return
                    } else {
                        ExportController.generateFullAccessConfig()
                    }

                    shareConnectionDrawer.headerText = qsTr("Connection to ") + serverSelector.text
                    shareConnectionDrawer.configContentHeaderText = qsTr("File with connection settings to ") + serverSelector.text

                    shareConnectionDrawer.openTriggered()
                    shareConnectionDrawer.contentVisible = true

                    PageController.showBusyIndicator(false)
                }
            }
        }
    }

    ShareConnectionDrawer {
        id: shareConnectionDrawer

        anchors.fill: parent
    }
}
