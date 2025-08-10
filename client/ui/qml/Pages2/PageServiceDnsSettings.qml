import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
        
        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                headerText: "AmneziaDNS"
                descriptionText: qsTr("A DNS service is installed on your server, and it is only accessible via VPN.\n") +
                                 qsTr("The DNS address is the same as the address of your server. You can configure DNS in the settings, under the connections tab.")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                textColor: AmneziaStyle.color.vibrantRed

                clickedFunction: function() {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected
                                && SettingsController.isAmneziaDnsEnabled()) {
                            PageController.showNotificationMessage(qsTr("Cannot remove AmneziaDNS from running server"))
                        } else {
                            PageController.goToPage(PageEnum.PageDeinstalling)
                            InstallController.removeProcessedContainer()
                        }
                    }
                    var noButtonFunction = function() {}

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }
        }
    }
}
