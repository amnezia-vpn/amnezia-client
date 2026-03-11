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

        // Оставляем пустую модель, так как мы перемещаем данные подписки прямо в красивую карточку в header
        model: 0

        header: ColumnLayout {
            width: listView.width
            spacing: 16

            BackButtonType {
                id: backButton
                objectName: "backButton"
                Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
            }

            HeaderTypeWithButton {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8

                actionButtonImage: "qrc:/images/controls/edit-3.svg"

                headerText: root.processedServer.name
                descriptionText: ApiAccountInfoModel.data("serviceDescription")

                actionButtonFunction: function() {
                    serverNameEditDrawer.openTriggered()
                }
            }

            // PREMIUM SUBSCRIPTION CARD
            Rectangle {
                Layout.fillWidth: true
                Layout.margins: 16
                implicitHeight: 180
                radius: 16

                // Градиентный фон карточки (золотой/темный в зависимости от статуса)
                gradient: Gradient {
                    GradientStop { 
                        position: 0.0
                        color: ApiAccountInfoModel.data("subscriptionStatus") !== "Expired" ? 
                               AmneziaStyle.color.translucentRichBrown : AmneziaStyle.color.translucentSlateGray
                    }
                    GradientStop { 
                        position: 1.0
                        color: AmneziaStyle.color.translucentOnyxBlack 
                    }
                }

                border.width: 1
                border.color: ApiAccountInfoModel.data("subscriptionStatus") !== "Expired" ? 
                              AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12

                    // Верхняя строка: Иконка и Статус
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Image {
                            source: ApiAccountInfoModel.data("subscriptionStatus") !== "Expired" ? 
                                    "qrc:/images/controls/shield-tick.svg" : "qrc:/images/controls/info.svg"
                            sourceSize: Qt.size(28, 28)
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: qsTr("Subscription Status")
                                font.pixelSize: 12
                                color: AmneziaStyle.color.mutedGray
                            }
                            Label {
                                text: ApiAccountInfoModel.data("subscriptionStatus")
                                font.pixelSize: 20
                                font.bold: true
                                color: ApiAccountInfoModel.data("subscriptionStatus") !== "Expired" ? 
                                       AmneziaStyle.color.goldenApricot : AmneziaStyle.color.vibrantRed
                            }
                        }
                    }

                    // Средняя строка: Дата окончания
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Image {
                                source: "qrc:/images/controls/history.svg"
                                sourceSize: Qt.size(20, 20)
                                opacity: 0.7
                            }

                            Label {
                                text: qsTr("Valid Until: ") + ApiAccountInfoModel.data("endDate")
                                font.pixelSize: 14
                                color: AmneziaStyle.color.lightGray
                                Layout.fillWidth: true
                            }
                        }

                        // Тонкий прогресс-бар времени подписки
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 32 // align with text
                            height: 4
                            radius: 2
                            color: Qt.rgba(255, 255, 255, 0.1)
                            visible: ApiAccountInfoModel.data("subscriptionStatus") !== "Expired"

                            Rectangle {
                                id: timeProgressBar
                                height: parent.height
                                radius: 2
                                color: {
                                    var pct = width / parent.width
                                    if (pct > 0.8) return AmneziaStyle.color.vibrantRed
                                    if (pct > 0.5) return AmneziaStyle.color.goldenApricot
                                    return "#10B981"
                                }
                                
                                width: {
                                    if (parent.width === 0) return 0
                                    var endStr = ApiAccountInfoModel.data("endDate")
                                    if (!endStr) return parent.width * 0.5
                                    
                                    var endDate = new Date(endStr)
                                    if (isNaN(endDate.getTime())) {
                                        var parts = endStr.split('.')
                                        if (parts.length === 3) {
                                            endDate = new Date(parts[2], parts[1]-1, parts[0])
                                        }
                                    }
                                    
                                    if (isNaN(endDate.getTime())) return parent.width * 0.5
                                    
                                    var now = new Date()
                                    var diff = endDate - now
                                    var daysLeft = Math.max(0, diff / (1000 * 60 * 60 * 24))
                                    var totalDays = 30 // assume 1 month
                                    var pct = Math.max(0.05, Math.min(1.0, (totalDays - daysLeft) / totalDays))
                                    return parent.width * pct
                                }
                                
                                Behavior on width { NumberAnimation { duration: 1000; easing.type: Easing.OutCubic } }
                            }
                        }
                    }

                    // Нижняя строка: Устройства
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Image {
                            source: "qrc:/images/controls/monitor.svg"
                            sourceSize: Qt.size(20, 20)
                            opacity: 0.7
                        }

                        Label {
                            text: qsTr("Active Connections: ")
                            font.pixelSize: 14
                            color: AmneziaStyle.color.lightGray
                        }

                        // Чипы устройств
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            
                            Repeater {
                                model: {
                                    // Parse the number of connections from the string, fallback to 0 if not parsable
                                    var countStr = ApiAccountInfoModel.data("connectedDevices")
                                    // The format might be something like "1/5" or just "1"
                                    var currentCount = 0
                                    if (countStr) {
                                        var parts = countStr.split("/");
                                        currentCount = parseInt(parts[0], 10) || 0;
                                    }
                                    return Math.min(currentCount, 5); // display up to 5 icons max to avoid overflow
                                }
                                
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    color: AmneziaStyle.color.goldenApricot
                                    
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 6
                                        height: 6
                                        radius: 3
                                        color: AmneziaStyle.color.onyxBlack
                                    }
                                }
                            }
                            
                            // If more than 5 devices, show a + count indicator
                            Label {
                                visible: {
                                    var countStr = ApiAccountInfoModel.data("connectedDevices")
                                    var currentCount = 0
                                    if (countStr) {
                                        var parts = countStr.split("/");
                                        currentCount = parseInt(parts[0], 10) || 0;
                                        return currentCount > 5;
                                    }
                                    return false;
                                }
                                text: {
                                    var countStr = ApiAccountInfoModel.data("connectedDevices")
                                    if (countStr) {
                                        var parts = countStr.split("/");
                                        var currentCount = parseInt(parts[0], 10) || 0;
                                        return "+" + (currentCount - 5);
                                    }
                                    return ""
                                }
                                font.pixelSize: 12
                                color: AmneziaStyle.color.goldenApricot
                            }
                            
                            Item { Layout.fillWidth: true } // spacer
                        }
                    }

                    // Кнопка продления подписки (YooKassa)
                    BasicButtonType {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        
                        text: qsTr("Renew Subscription")
                        defaultColor: AmneziaStyle.color.goldenApricot
                        hoveredColor: Qt.lighter(AmneziaStyle.color.goldenApricot, 1.1)
                        pressedColor: Qt.darker(AmneziaStyle.color.goldenApricot, 1.1)
                        textColor: AmneziaStyle.color.midnightBlack
                        
                        clickedFunc: function() {
                            PageController.showBusyIndicator(true)
                            var success = ApiConfigsController.renewYookassaSubscription("premium")
                            PageController.showBusyIndicator(false)
                            if (!success) {
                                PageController.showNotificationMessage(qsTr("Failed to initiate payment. Please try again later."))
                            }
                        }
                    }
                }
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
                            if (ApiConfigsController.deactivateDevice(false)) {
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
                            InstallController.removeProcessedServer()
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
