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

    property bool pendingOpenQrPageAfterCamera: false
    property bool waitingSettingsReturnForQrPage: false

    function proceedOpenQrPairingPage() {
        PageController.goToPage(PageEnum.PageSettingsApiQrPairingSend)
        pendingOpenQrPageAfterCamera = false
        waitingSettingsReturnForQrPage = false
    }

    function showCameraDeniedDrawer() {
        showQuestionDrawer(
                    qsTr("Camera access is required"),
                    qsTr("Allow camera access to scan the pairing QR code. You can enable it in the system settings for Amnezia VPN."),
                    qsTr("Open settings"),
                    qsTr("Cancel"),
                    function() {
                        PairingUiController.openPairingCameraAppSettings()
                    },
                    function() {
                        waitingSettingsReturnForQrPage = false
                    })
    }

    function tryResumeQrPageAfterCameraSettings() {
        if (!waitingSettingsReturnForQrPage || !root.visible) {
            return
        }
        if (PairingUiController.isPairingCameraAccessGranted()) {
            proceedOpenQrPairingPage()
        }
    }

    function openAddDeviceViaQr() {
        if (Qt.platform.os !== "android" && Qt.platform.os !== "ios") {
            PageController.goToPage(PageEnum.PageSettingsApiQrPairingSend)
            return
        }
        if (PairingUiController.isPairingCameraAccessGranted()) {
            proceedOpenQrPairingPage()
            return
        }
        pendingOpenQrPageAfterCamera = true
        PairingUiController.requestPairingCameraAccess()
    }

    onVisibleChanged: {
        if (!visible) {
            pendingOpenQrPageAfterCamera = false
            waitingSettingsReturnForQrPage = false
        }
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                return
            }
            root.tryResumeQrPageAfterCameraSettings()
        }
    }

    Connections {
        target: SettingsController

        enabled: Qt.platform.os === "android"

        function onActivityResumed() {
            root.tryResumeQrPageAfterCameraSettings()
        }
    }

    Connections {
        target: PairingUiController

        function onPairingCameraAccessFinished(granted) {
            if (!root.pendingOpenQrPageAfterCamera) {
                return
            }
            root.pendingOpenQrPageAfterCamera = false
            if (granted) {
                root.proceedOpenQrPairingPage()
            } else {
                root.waitingSettingsReturnForQrPage = true
                root.showCameraDeniedDrawer()
            }
        }

        function onPhonePairingSucceeded() {
            var serverId = ServersUiController.getServerId(ServersUiController.processedServerIndex)
            if (serverId.length === 0) {
                return
            }
            SubscriptionUiController.getAccountInfo(serverId, true)
            SubscriptionUiController.updateApiDevicesModel()
            if (!root.visible) {
                return
            }
            const label = PairingUiController.lastSuccessfulPhonePairingDisplayName
            if (label.length > 0) {
                PageController.showNotificationMessage(
                            qsTr("%1 has been added to your subscription").arg(label))
            } else {
                PageController.showNotificationMessage(qsTr("New device has been added to your subscription"))
            }
        }

    }

    ListViewType {
        id: listView

        anchors.fill: parent
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
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
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 20

                implicitHeight: 52

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Add Device via QR Code")

                clickedFunc: function() {
                    root.openAddDeviceViaQr()
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: AmneziaStyle.color.mutedGray
                text: qsTr("On the other device, tap + at the bottom, then choose Connect to Amnezia Premium")
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
                    if (isCurrentDevice && ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot unlink device during active connection"))
                        return
                    }

                    var headerText = qsTr("Are you sure you want to unlink this device?")
                    var descriptionText = qsTr("This will unlink the device from your subscription. You can reconnect it anytime by pressing \"Reload API config\" in subscription settings on device.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        var serverId = ServersUiController.getServerId(ServersUiController.processedServerIndex)
                        Qt.callLater(deactivateExternalDevice, serverId, supportTag, countryCode)
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
                Layout.topMargin: 8

                text: qsTr("Configuration Files: %1").arg(ApiAccountInfoModel.data("configurationFilesCount"))
                descriptionText: qsTr("Generated configuration files also count towards the device limit")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    SubscriptionUiController.updateApiCountryModel()
                    PageController.goToPage(PageEnum.PageSettingsApiNativeConfigs)
                }
            }

            DividerType {}
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
