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

    function getConfigFilesCount() {
        try {
            var arr = ApiAccountInfoModel.getIssuedConfigsInfo()
            if (!arr) return 0

            var count = 0
            for (var i = 0; i < arr.length; i++) {
                var item = arr[i]
                if (item && item["source_type"] === "country_config") {
                    ++count
                }
            }
            return count
        } catch (e) {
            return 0
        }
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

                visible: GC.isMobile()

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

            SmallTextType {
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.fillWidth: true

                text: qsTr("On the other device, tap + at the bottom → Connect to Amnezia Premium")
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

        footer: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 6

                text: qsTr("Configuration Files: {%1}").arg(root.getConfigFilesCount())
                descriptionText: qsTr("Generated configuration files also count towards the device limit")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    ApiSettingsController.updateApiCountryModel()
                    PageController.goToPage(PageEnum.PageSettingsApiNativeConfigs)
                }
            }

            DividerType {}

            WarningType {
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 8
                Layout.fillWidth: true

                textString: qsTr("The Support tag is available on the Support page. In older versions: tap + in the bottom bar, then More (...) in the top-right.")
                iconPath: "qrc:/images/controls/alert-circle.svg"
            }
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
