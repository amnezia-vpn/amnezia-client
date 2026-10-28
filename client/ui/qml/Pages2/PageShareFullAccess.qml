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

    property string headerText: ""
    property string configContentHeaderText: ""

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: shareButton.top
        anchors.bottomMargin: 24

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            width: flickable.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Full access to the server and VPN")
                description: qsTr("We recommend that you use full access to the server only for your own additional devices.\n") +
                             qsTr("If you share full access with other people, they can remove and add protocols and services to the server, which will cause the VPN to work incorrectly for all users. ")
            }

            DropDownType {
                id: serverSelector

                property int currentIndex: 0

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                drawerHeight: 0.4375

                descriptionText: qsTr("Server")
                headerText: qsTr("Server")

                listView: RadioListType {
                    id: serverSelectorListView

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

                    onActivated: {
                        applySelection()
                        serverSelector.close()
                    }

                    Component.onCompleted: {
                        selectedIndex = ServersUiController.isServerHasWriteAccess(ServersUiController.defaultServerId) ?
                                    proxyServersModel.mapFromSource(ServersUiController.getServerIndexById(ServersUiController.defaultServerId)) : 0
                        applySelection()
                    }

                    function applySelection() {
                        const serverId = ServersUiController.getServerId(proxyServersModel.mapToSource(selectedIndex))
                        ServersUiController.setProcessedServerId(serverId)

                        serverSelector.currentIndex = selectedIndex
                        serverSelector.text = ServersUiController.serverName(serverId)

                        root.headerText = qsTr("Accessing ") + serverSelector.text
                        root.configContentHeaderText = qsTr("File with accessing settings to ") + serverSelector.text
                    }
                }
            }
        }
    }

    ButtonType {
        id: shareButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        text: qsTr("Share")
        leftImageSource: "qrc:/images/controls/share-2.svg"

        onClicked: {
            PageController.showBusyIndicator(true)

            if (Qt.platform.os === "android" && !SystemController.isAuthenticated()) {
                PageController.showBusyIndicator(false)
                ExportController.exportErrorOccurred(qsTr("Access error!"))
                return
            } else {
                ExportController.generateFullAccessConfig(ServersUiController.processedServerId)
            }

            PageController.showBusyIndicator(false)

            PageController.goToShareConnectionPage(root.headerText, root.configContentHeaderText, "", ".vpn", "amnezia_config")
        }
    }
}
