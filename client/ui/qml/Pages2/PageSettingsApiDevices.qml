import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore
import QRCodeReader 1.0

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

    function isAtDeviceLimit() {
        var maxDeviceCount = ApiAccountInfoModel.data("maxDeviceCount")
        return listView.count >= maxDeviceCount
    }

    ListViewType {
        id: listView

        anchors.fill: parent
        anchors.topMargin: 20
        anchors.bottomMargin: 24

        model: ApiDevicesModel

        header: ColumnLayout {
            width: listView.width

            BackButtonType {
                id: backButton
            }

            BaseHeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Active Devices")
                descriptionText: qsTr("Manage currently connected devices")
            }

            BasicButtonType {
                id: addDeviceQrButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                //visible: GC.isMobile()

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Add device by QR code")

                clickedFunc: function() {
                    if (root.isAtDeviceLimit()) {
                        PageController.goToPage(PageEnum.PageSettingsApiDevicesLimit)
                    } else {
                        PageController.goToPage(PageEnum.PageSettingsApiAddDeviceScan)
                    }
                }
            }

            WarningType {
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.fillWidth: true

                textString: qsTr("You can find the identifier on the Support tab or, for older versions of the app, "
                                 + "by tapping '+' and then the three dots at the top of the page.")

                iconPath: "qrc:/images/controls/alert-circle.svg"
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 6

                text: osVersion + (isCurrentDevice ? qsTr(" (current device)") : "")
                descriptionText: qsTr("Support tag: ") + "\n" + supportTag + "\n" + qsTr("Last updated: ") + lastUpdate
                rightImageSource: "qrc:/images/controls/trash.svg"

                clickedFunction: function() {
                    if (isCurrentDevice && ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot unlink device during active connection"))
                        return
                    }

                    var headerText = qsTr("Are you sure you want to unlink this device?")
                    var descriptionText = qsTr("This will unlink the device from your subscription. You can reconnect it anytime by pressing \"Reload API config\" in subscription settings on device.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        Qt.callLater(deactivateExternalDevice, supportTag, countryCode)
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {}
        }

        Connections {
            target: TransferController

            function onPostStarted() {
                PageController.showBusyIndicator(true)
            }

            function onPostSucceeded() {
                PageController.showBusyIndicator(false)
                ApiSettingsController.getAccountInfo(true)
                PageController.showNotificationMessage(qsTr("New device added to subscription"))
            }

            function onPostFailed(message) {
                PageController.showBusyIndicator(false)
                PageController.showErrorMessage(message)
            }

            function onScannerShouldStop() {}
        }
    }

    function deactivateExternalDevice(supportTag, countryCode) {
        PageController.showBusyIndicator(true)
        if (ApiConfigsController.deactivateExternalDevice(supportTag, countryCode)) {
            ApiSettingsController.getAccountInfo(true)
        }
        PageController.showBusyIndicator(false)
    }
}
