import QtQuick

import PageEnum 1.0

import "./"

PageType {
    id: root

    Component.onCompleted: {
        PageController.showNotificationMessage(qsTr("Раздельное туннелирование по сайтам перенесено в VIP-маршрутизацию"))
        PageController.goToPage(PageEnum.PageSettingsVipRoutingProfiles)
    }
}
