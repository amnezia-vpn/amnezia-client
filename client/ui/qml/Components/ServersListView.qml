import QtQuick
import QtQuick.Controls

import PageEnum 1.0
import Style 1.0

import "../Controls2"

ListViewType {
    id: root

    signal serverSettingsOpened()

    leftMargin: 16
    rightMargin: 16
    bottomMargin: 16

    model: ServersModel

    function openServerSettings(serverId) {
        ServersUiController.setProcessedServerId(serverId)

        if (ServersUiController.isServerFromApi(ServersUiController.processedServerId)) {
            if (ServersUiController.isServerCountrySelectionAvailable(ServersUiController.processedServerId)) {
                root.serverSettingsOpened()
                PageController.goToPage(PageEnum.PageSettingsApiAvailableCountries)
            } else {
                PageController.showBusyIndicator(true)
                let result = SubscriptionUiController.getAccountInfo(ServersUiController.processedServerId, false)
                PageController.showBusyIndicator(false)
                if (!result) {
                    return
                }

                root.serverSettingsOpened()
                PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
            }
        } else {
            root.serverSettingsOpened()
            PageController.goToPage(PageEnum.PageSettingsServerInfo)
        }
    }

    delegate: ServerListItem {
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

        checked: serverId === ServersUiController.defaultServerId

        onClicked: {
            if (ConnectionController.isConnected) {
                PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                return
            }

            ServersUiController.setDefaultServerAtIndex(index)
        }

        onSettingsRequested: root.openServerSettings(serverId)
    }
}
