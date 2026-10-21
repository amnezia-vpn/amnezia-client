import QtQuick
import QtQuick.Controls

import PageEnum 1.0
import Style 1.0

import "../Controls2"

ListViewType {
    id: root

    property int selectedIndex: ServersUiController.getServerIndexById(ServersUiController.defaultServerId)

    signal serverSettingsOpened()

    leftMargin: 16
    rightMargin: 16
    bottomMargin: 16

    model: ServersModel

    Connections {
        target: ServersUiController
        function onDefaultServerIdChanged() {
            root.selectedIndex = ServersUiController.getServerIndexById(ServersUiController.defaultServerId)
        }
    }

    ButtonGroup {
        id: serversRadioButtonGroup
        objectName: "serversRadioButtonGroup"
    }

    function openServerSettings(serverId) {
        ServersUiController.setProcessedServerId(serverId)

        if (ServersUiController.isServerFromApi(ServersUiController.processedServerId)) {
            if (ServersUiController.isServerCountrySelectionAvailable(ServersUiController.processedServerId)) {
                PageController.goToPage(PageEnum.PageSettingsApiAvailableCountries)
            } else {
                PageController.showBusyIndicator(true)
                let result = SubscriptionUiController.getAccountInfo(ServersUiController.processedServerId, false)
                PageController.showBusyIndicator(false)
                if (!result) {
                    return
                }

                PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
            }
        } else {
            PageController.goToPage(PageEnum.PageSettingsServerInfo)
        }

        root.serverSettingsOpened()
    }

    delegate: ServerListItem {
        objectName: "serverRadioButton"

        readonly property bool subscriptionAlert: isServerFromGatewayApi && (isSubscriptionExpired || isSubscriptionExpiringSoon)

        width: root.width - root.leftMargin - root.rightMargin

        text: name
        subtitle: subscriptionAlert
                  ? (isSubscriptionExpired ? qsTr("Subscription expired. Please renew") : qsTr("Subscription expiring soon"))
                  : serverDescription
        subtitleColor: subscriptionAlert
                       ? (isSubscriptionExpired ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.statusWarning)
                       : AmneziaStyle.color.textTertiary
        showWarning: ServersUiController.serverHasOutdatedAwgContainer(serverId)
        showSeparator: index < root.count - 1

        checked: index === root.selectedIndex
        checkable: !ConnectionController.isConnected

        ButtonGroup.group: serversRadioButtonGroup

        onClicked: {
            if (ConnectionController.isConnected) {
                PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                return
            }

            root.selectedIndex = index

            ServersUiController.setDefaultServerAtIndex(index)
        }

        onSettingsRequested: root.openServerSettings(serverId)
    }
}
