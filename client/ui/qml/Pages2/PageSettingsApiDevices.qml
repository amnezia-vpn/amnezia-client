import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

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

    ListViewType {
        id: listView

        anchors.fill: parent

        model: ApiDevicesModel

        header: ColumnLayout {
            width: listView.width

            z: 2

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Active Devices")
                description: qsTr("Manage currently connected devices")
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.bottomMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("You can find the identifier on the Support tab or, for older versions of the app, "
                                 + "by tapping '+' and then the three dots at the top of the page.")
            }
        }

        footer: Item {
            width: listView.width
            height: 16 + PageController.safeAreaBottomMargin
        }

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: osVersion + (isCurrentDevice ? qsTr(" (current device)") : "")
                subtitle: qsTr("Support tag: ") + "\n" + supportTag + "\n" + qsTr("Last updated: ") + lastUpdate
                maximumLineCount: 3
                trailingIconSource: "qrc:/images/controls/trash.svg"

                onClicked: {
                    if (isCurrentDevice && ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot unlink device during active connection"))
                        return
                    }

                    if (isCurrentDevice && LocalProxyController.isEnabled
                            && LocalProxyController.ownerId === ServersUiController.processedServerId) {
                        PageController.showNotificationMessage(qsTr("Cannot unlink device while local proxy is running"))
                        return
                    }

                    var headerText = qsTr("Are you sure you want to unlink this device?")
                    var descriptionText = qsTr("This will unlink the device from your subscription. You can reconnect it anytime by pressing \"Reload API config\" in subscription settings on device.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        var serverId = ServersUiController.processedServerId
                        Qt.callLater(deactivateExternalDevice, serverId, supportTag, countryCode)
                    }
                    var noButtonFunction = function() {
                    }

                    Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }

    function deactivateExternalDevice(serverId, supportTag, countryCode) {
        PageController.showBusyIndicator(true)
        if (SubscriptionUiController.deactivateExternalDevice(serverId, supportTag, countryCode)) {
            SubscriptionUiController.getAccountInfo(serverId, true)
        }
        PageController.showBusyIndicator(false)
    }
}
