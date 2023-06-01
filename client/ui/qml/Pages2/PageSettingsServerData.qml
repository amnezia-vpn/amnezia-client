import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"

PageType {
    id: root

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            LabelWithButtonType {
                Layout.fillWidth: true

                text: "Clear Amnezia cache"
                descriptionText: "May be needed when changing other settings"

                clickedFunction: function() {
                    ContainersModel.clearCachedProfiles()
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: "Remove server from application"
                textColor: "#EB5757"

                clickedFunction: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed && ConnectionController.isConnected()) {
                        ConnectionController.closeVpnConnection()
                    }
                    ServersModel.removeServer()
                    if (!ServersModel.getServersCount()) {
                        PageController.replaceStartPage()
                    } else {
                        goToStartPage()
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: "Clear server from Amnezia software"
                textColor: "#EB5757"

                clickedFunction: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed && ConnectionController.isConnected()) {
                        ConnectionController.closeVpnConnection()
                    }
                    ContainersModel.removeAllContainers()
                }
            }

            DividerType {}
        }
    }
}
