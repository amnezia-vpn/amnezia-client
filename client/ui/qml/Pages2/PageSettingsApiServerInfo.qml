import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

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

    property list<QtObject> labelsModel: [
        statusObject,
        endDateObject,
        deviceCountObject
    ]

    QtObject {
        id: statusObject

        readonly property string title: qsTr("Subscription Status")
        readonly property string contentKey: "subscriptionStatus"
        readonly property string objectImageSource: "qrc:/images/controls/info.svg"
        readonly property bool isRichText: true
    }

    QtObject {
        id: endDateObject

        readonly property string title: qsTr("Valid Until")
        readonly property string contentKey: "endDate"
        readonly property string objectImageSource: "qrc:/images/controls/history.svg"
        readonly property bool isRichText: false
    }

    QtObject {
        id: deviceCountObject

        readonly property string title: qsTr("Active Connections")
        readonly property string contentKey: "connectedDevices"
        readonly property string objectImageSource: "qrc:/images/controls/monitor.svg"
        readonly property bool isRichText: false
    }

    property var processedServer

    property bool isSubscriptionExpired: false
    property bool isSubscriptionExpiringSoon: false
    property bool isSubscriptionRenewalAvailable: false
    property bool isInAppPurchase: false
    property bool isVisibleForAmneziaFree: false
    property string serviceDescription: ""

    readonly property bool showSubscriptionNote: root.isSubscriptionExpired || root.isSubscriptionExpiringSoon
    readonly property bool showRenewButton: root.showSubscriptionNote
                                            && root.isSubscriptionRenewalAvailable && !root.isInAppPurchase
    readonly property bool showRenewLink: !root.showSubscriptionNote
                                          && root.isSubscriptionRenewalAvailable && !root.isInAppPurchase

    function updateSubscriptionState() {
        root.isSubscriptionExpired = ApiAccountInfoModel.data("isSubscriptionExpired")
        root.isSubscriptionExpiringSoon = ApiAccountInfoModel.data("isSubscriptionExpiringSoon")
        root.isSubscriptionRenewalAvailable = ApiAccountInfoModel.data("isSubscriptionRenewalAvailable")
        root.isInAppPurchase = ApiAccountInfoModel.data("isInAppPurchase")
        root.isVisibleForAmneziaFree = ApiAccountInfoModel.data("isComponentVisible")
        root.serviceDescription = ApiAccountInfoModel.data("serviceDescription") || ""
    }

    Component.onCompleted: {
        root.updateSubscriptionState()
    }

    Connections {
        target: ApiAccountInfoModel

        function onModelReset() {
            root.updateSubscriptionState()
        }
    }

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ServersModel

        function onModelReset() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "serverId"
                value: ServersUiController.processedServerId
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    FlickableType {
        id: flickable

        anchors.fill: parent

        contentHeight: Math.max(content.implicitHeight, flickable.height)

        ColumnLayout {
            id: content

            width: flickable.width
            height: flickable.contentHeight

            spacing: 0

            NavigationBarType {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true

                title: root.processedServer != null ? root.processedServer.name : ""
                description: root.serviceDescription

                IconButtonType {
                    size: IconButtonType.Small
                    icon.source: "qrc:/images/controls/edit-3.svg"

                    onClicked: serverNameEditDrawer.open()
                }
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8

                visible: root.showSubscriptionNote

                text: root.isSubscriptionExpired ? qsTr("Subscription expired")
                                                 : qsTr("Subscription expiring soon")
                color: root.isSubscriptionExpired ? AmneziaStyle.color.statusDestructive
                                                  : AmneziaStyle.color.textAccent
            }

            ButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.showRenewButton

                text: qsTr("Renew subscription")

                onClicked: {
                    SubscriptionUiController.getRenewalLink(ServersUiController.processedServerId)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 0

                Repeater {
                    model: root.labelsModel

                    delegate: ListItemType {
                        id: infoRow

                        required property var modelData

                        property string value: ApiAccountInfoModel.data(infoRow.modelData.contentKey) || ""

                        Layout.fillWidth: true

                        visible: infoRow.value !== ""

                        interactive: false
                        showSeparator: false
                        leadingSlotWidth: 24

                        text: infoRow.modelData.title
                        icon.source: infoRow.modelData.objectImageSource

                        Connections {
                            target: ApiAccountInfoModel

                            function onModelReset() {
                                infoRow.value = ApiAccountInfoModel.data(infoRow.modelData.contentKey) || ""
                            }
                        }

                        SmallTextType {
                            Layout.maximumWidth: infoRow.availableWidth * 0.5
                            Layout.rightMargin: 4

                            text: infoRow.value
                            textFormat: infoRow.modelData.isRichText ? Text.RichText : Text.PlainText
                            color: AmneziaStyle.color.textSecondary
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            ButtonType {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24

                visible: root.showRenewLink

                size: ButtonType.Medium
                variant: ButtonType.Text
                leftImageSource: "qrc:/images/controls/refresh-cw.svg"
                text: qsTr("Renew subscription")

                onClicked: {
                    SubscriptionUiController.getRenewalLink(ServersUiController.processedServerId)
                }
            }

            NotificationType {
                id: warning

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                variant: NotificationType.Warning
                text: qsTr("Configurations have been updated for some countries. Download and install the updated configuration files")

                visible: ApiCountryModel.hasExpiredWorkerConfigs
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 0

                ListItemType {
                    id: localProxyItem

                    Layout.fillWidth: true

                    visible: LocalProxyController.isSupported && ServersUiController.processedServerIsPremium

                    text: qsTr("Local proxy")
                    subtitle: {
                        if (!LocalProxyController.isEnabled) {
                            return qsTr("Off")
                        }
                        if (LocalProxyController.ownerId !== ServersUiController.processedServerId) {
                            return qsTr("In use by another server")
                        }
                        return qsTr("Running: 127.0.0.1:%1").arg(LocalProxyController.activePort > 0 ? LocalProxyController.activePort : LocalProxyController.port)
                    }
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsLocalProxy)
                    }
                }

                ListItemType {
                    id: vpnKey

                    Layout.fillWidth: true

                    visible: root.isVisibleForAmneziaFree

                    text: qsTr("Subscription Key")
                    subtitle: qsTr("For connecting on other devices")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsApiSubscriptionKey)
                        PageController.showBusyIndicator(true)

                        SubscriptionUiController.prepareVpnKeyExport(ServersUiController.processedServerId)

                        PageController.showBusyIndicator(false)
                    }
                }

                ListItemType {
                    Layout.fillWidth: true

                    visible: root.isVisibleForAmneziaFree

                    text: qsTr("Configuration Files")
                    subtitle: qsTr("Manage configuration files")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        SubscriptionUiController.updateApiCountryModel(ServersUiController.processedServerId)
                        PageController.goToPage(PageEnum.PageSettingsApiNativeConfigs)
                    }
                }

                ListItemType {
                    Layout.fillWidth: true

                    visible: root.isVisibleForAmneziaFree

                    text: qsTr("Active Devices")
                    subtitle: qsTr("Manage currently connected devices")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        SubscriptionUiController.updateApiDevicesModel()
                        PageController.goToPage(PageEnum.PageSettingsApiDevices)
                    }
                }

                ListItemType {
                    Layout.fillWidth: true

                    text: qsTr("Support")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsApiSupport)
                    }
                }

                ListItemType {
                    Layout.fillWidth: true

                    visible: root.isVisibleForAmneziaFree

                    text: qsTr("How to connect on another device")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsApiInstructions)
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 24
            }

            ButtonType {
                id: resetButton

                Layout.alignment: Qt.AlignHCenter

                size: ButtonType.Medium
                variant: ButtonType.Text
                destructive: true
                text: qsTr("Reload API config")

                onClicked: {
                    var headerText = qsTr("Reload API config?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot reload API config during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            SubscriptionUiController.updateServiceFromGateway(ServersUiController.processedServerId, "", "", true)
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ButtonType {
                id: revokeButton

                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 12

                visible: root.isVisibleForAmneziaFree

                size: ButtonType.Medium
                variant: ButtonType.Text
                destructive: true
                text: qsTr("Unlink this device")

                onClicked: {
                    var headerText = qsTr("Are you sure you want to unlink this device?")
                    var descriptionText = qsTr("This will unlink the device from your subscription. You can reconnect it anytime by pressing \"Reload API config\" in subscription settings on device.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot unlink device during active connection"))
                        } else if (LocalProxyController.isEnabled
                                   && LocalProxyController.ownerId === ServersUiController.processedServerId) {
                            PageController.showNotificationMessage(qsTr("Cannot unlink device while local proxy is running"))
                        } else {
                            PageController.showBusyIndicator(true)
                            if (SubscriptionUiController.deactivateDevice(ServersUiController.processedServerId)) {
                                SubscriptionUiController.getAccountInfo(ServersUiController.processedServerId, true)
                            }
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ButtonType {
                id: removeButton

                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 12
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                size: ButtonType.Medium
                variant: ButtonType.Text
                destructive: true
                text: qsTr("Remove from application")

                onClicked: {
                    var headerText = qsTr("Remove from application?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                        } else if (LocalProxyController.isEnabled
                                   && LocalProxyController.ownerId === ServersUiController.processedServerId) {
                            PageController.showNotificationMessage(qsTr("Cannot remove server while local proxy is running"))
                        } else {
                            PageController.showBusyIndicator(true)
                            SubscriptionUiController.removeServer(ServersUiController.processedServerId)
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }

    RenameServerDrawer {
        id: serverNameEditDrawer

        serverNameText: root.processedServer != null ? root.processedServer.name : ""
    }
}
