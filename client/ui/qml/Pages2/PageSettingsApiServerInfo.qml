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
    }

    QtObject {
        id: endDateObject

        readonly property string title: qsTr("Valid Until")
        readonly property string contentKey: "endDate"
        readonly property string objectImageSource: "qrc:/images/controls/history.svg"
    }

    QtObject {
        id: deviceCountObject

        readonly property string title: qsTr("Active Connections")
        readonly property string contentKey: "connectedDevices"
        readonly property string objectImageSource: "qrc:/images/controls/monitor.svg"
    }

    property var processedServer

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        model: labelsModel

        header: ColumnLayout {
            width: listView.width

            spacing: 4

            BackButtonType {
                id: backButton
                objectName: "backButton"

                Layout.topMargin: 20
            }

            HeaderTypeWithButton {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 10

                actionButtonImage: "qrc:/images/controls/edit-3.svg"

                headerText: root.processedServer.name
                descriptionText: ApiAccountInfoModel.data("serviceDescription")

                actionButtonFunction: function() {
                    serverNameEditDrawer.openTriggered()
                }
            }
        }

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            Connections {
                target: ApiAccountInfoModel

                function onModelReset() {
                    delegateItem.rightText = ApiAccountInfoModel.data(contentKey)
                }
            }

            LabelWithImageType {
                id: delegateItem

                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: objectImageSource
                leftText: title
                rightText: ApiAccountInfoModel.data(contentKey)

                visible: rightText !== ""
            }
        }

        footer: ColumnLayout {
            id: footer

            width: listView.width
            spacing: 0

            readonly property bool isVisibleForAmneziaFree: ApiAccountInfoModel.data("isComponentVisible")

            SwitcherType {
                id: switcher

                readonly property bool isVlessProtocol: ApiConfigsController.isVlessProtocol()

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                visible: ApiAccountInfoModel.data("isProtocolSelectionSupported")

                text: qsTr("Use VLESS protocol")
                checked: switcher.isVlessProtocol
                onToggled: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot change protocol during active connection"))
                    } else {
                        PageController.showBusyIndicator(true)
                        ApiConfigsController.setCurrentProtocol(switcher.isVlessProtocol ? "awg" : "vless")
                        ApiConfigsController.updateServiceFromGateway(ServersModel.processedIndex, "", "", true)
                        PageController.showBusyIndicator(false)
                    }
                }
            }

            WarningType {
                id: warning

                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.fillWidth: true

                backGroundColor: AmneziaStyle.color.translucentRichBrown

                textString: qsTr("Configurations have been updated for some countries. Download and install the updated configuration files")

                iconPath: "qrc:/images/controls/alert-circle.svg"

                visible: {
                    for (let i = 0; i < ApiCountryModel.count; ++i) {
                        if (ApiCountryModel.get(i).isWorkerExpired)
                            return true;
                    }
                    return false;
                }
            }

            LabelWithButtonType {
                id: vpnKey

                Layout.fillWidth: true
                Layout.topMargin: warning.visible ? 16 : 32

                visible: footer.isVisibleForAmneziaFree

                text: qsTr("Subscription Key")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsApiSubscriptionKey)
                    PageController.showBusyIndicator(true)

                    ApiConfigsController.prepareVpnKeyExport()

                    PageController.showBusyIndicator(false)
                    
                    // Navigate to PageShareConnection page
                    //PageController.goToPage(PageEnum.PageShareConnection)
                }
            }

            DividerType {
                visible: footer.isVisibleForAmneziaFree
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: footer.isVisibleForAmneziaFree

                text: qsTr("Configuration Files")

                descriptionText: qsTr("Manage configuration files")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    ApiSettingsController.updateApiCountryModel()
                    PageController.goToPage(PageEnum.PageSettingsApiNativeConfigs)
                }
            }

            DividerType {
                visible: footer.isVisibleForAmneziaFree
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: footer.isVisibleForAmneziaFree

                text: qsTr("Active Devices")

                descriptionText: qsTr("Manage currently connected devices")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    ApiSettingsController.updateApiDevicesModel()
                    PageController.goToPage(PageEnum.PageSettingsApiDevices)
                }
            }

            DividerType {
                visible: footer.isVisibleForAmneziaFree
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: footer.isVisibleForAmneziaFree ? 0 : 32

                text: qsTr("Support")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsApiSupport)
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: footer.isVisibleForAmneziaFree

                text: qsTr("How to connect on another device")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsApiInstructions)
                }
            }

            DividerType {
                visible: footer.isVisibleForAmneziaFree
            }

            BasicButtonType {
                id: resetButton
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.vibrantRed

                text: qsTr("Reload API config")

                clickedFunc: function() {
                    var headerText = qsTr("Reload API config?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot reload API config during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            ApiConfigsController.updateServiceFromGateway(ServersModel.processedIndex, "", "", true)
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            BasicButtonType {
                id: revokeButton
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                visible: footer.isVisibleForAmneziaFree

                defaultColor: "transparent"
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.vibrantRed

                text: qsTr("Unlink this device")

                clickedFunc: function() {
                    var headerText = qsTr("Are you sure you want to unlink this device?")
                    var descriptionText = qsTr("This will unlink the device from your subscription. You can reconnect it anytime by pressing \"Reload API config\" in subscription settings on device.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot unlink device during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            if (ApiConfigsController.deactivateDevice()) {
                                ApiSettingsController.getAccountInfo(true)
                            }
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            BasicButtonType {
                id: removeButton
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.vibrantRed

                text: qsTr("Remove from application")

                clickedFunc: function() {
                    var headerText = qsTr("Remove from application?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            if (ApiConfigsController.deactivateDevice()) {
                                InstallController.removeProcessedServer()
                            }
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

        anchors.fill: parent
        expandedHeight: parent.height * 0.35

        serverNameText: root.processedServer.name
    }
}
