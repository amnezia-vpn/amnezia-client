import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType2 {
    id: root

    property bool isAppSplitTinnelingEnabled: Qt.platform.os === "windows" || Qt.platform.os === "android"
    property bool canManageRoutingProfiles: FBLinkController.canManageRoutingProfiles
    property bool canUseAppSplitTunneling: FBLinkController.canUseAppSplitTunneling

    function openVipFeature(page, isAllowed) {
        if (isAllowed) {
            PageController.goToPage(page)
        } else {
            PageController.showNotificationMessage(qsTr("Функция доступна только в VIP"))
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
        }
        root.closeTriggered()
    }

    anchors.fill: parent
    expandedHeight: parent.height * 0.9

    expandedStateContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Header2Type {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("Маршрутизация")
            descriptionText:  qsTr("Управляйте сайтами через VIP-профили и раздельным туннелированием приложений")
        }

        LabelWithButtonType {
            id: siteRoutingButton
            Layout.fillWidth: true
            Layout.topMargin: 16

            text: qsTr("Маршрутизация сайтов (VIP)")
            descriptionText: root.canManageRoutingProfiles
                ? qsTr("Системные пресеты и ваши профили")
                : qsTr("Доступно только в VIP")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
                root.openVipFeature(PageEnum.PageSettingsVipRoutingProfiles, root.canManageRoutingProfiles)
            }
        }

        DividerType {
        }

        LabelWithButtonType {
            id: appSplitTunnelingSwitch
            visible: isAppSplitTinnelingEnabled

            Layout.fillWidth: true

            text: qsTr("Раздельное туннелирование по приложениям")
            descriptionText: root.canUseAppSplitTunneling
                ? (AppSplitTunnelingModel.isTunnelingEnabled ? qsTr("Включено") : qsTr("Выключено"))
                : qsTr("Доступно только в VIP")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
                root.openVipFeature(PageEnum.PageSettingsAppSplitTunneling, root.canUseAppSplitTunneling)
            }
        }

        DividerType {
            visible: isAppSplitTinnelingEnabled
        }
    }
}
