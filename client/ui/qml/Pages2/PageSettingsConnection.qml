import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root

    property bool isAppSplitTinnelingEnabled: Qt.platform.os === "windows" || Qt.platform.os === "android"
    property bool canUseAppSplitTunneling: FBLinkController.canUseAppSplitTunneling
    property bool canManageRoutingProfiles: FBLinkController.canManageRoutingProfiles

    function goToVipFeature(page, isAllowed) {
        if (isAllowed) {
            PageController.goToPage(page)
        } else {
            PageController.showNotificationMessage(qsTr("Функция доступна только в VIP"))
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        header: ColumnLayout {

            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Подключение")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout { // TODO(CyAn84): add DelegateChooser when have migrated to 6.9

            width: listView.width



            LabelWithButtonType {
                id: dnsServersButton

                Layout.fillWidth: true

                text: qsTr("DNS-серверы")
                descriptionText: qsTr("Настройка DNS для разрешения доменных имён")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsDns)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: routingProfilesButton

                Layout.fillWidth: true

                text: qsTr("Маршрутизация сайтов (VIP)")
                descriptionText: root.canManageRoutingProfiles
                    ? qsTr("Системные пресеты и ваши профили маршрутизации")
                    : qsTr("Доступно только в VIP")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    root.goToVipFeature(PageEnum.PageSettingsVipRoutingProfiles, root.canManageRoutingProfiles)
                }
            }

            DividerType {}

        }

        footer: ColumnLayout { // TODO(CyAn84): move to delegate,add DelegateChooser when have migrated to 6.9

            width: listView.width

            LabelWithButtonType {
                id: splitTunnelingButton2

                visible: root.isAppSplitTinnelingEnabled

                Layout.fillWidth: true

                text: qsTr("Раздельное туннелирование (приложения)")
                descriptionText: root.canUseAppSplitTunneling
                    ? qsTr("Выбор приложений, работающих через VPN")
                    : qsTr("Доступно только в VIP")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    root.goToVipFeature(PageEnum.PageSettingsAppSplitTunneling, root.canUseAppSplitTunneling)
                }
            }

            DividerType {
                visible: root.isAppSplitTinnelingEnabled
            }

            LabelWithButtonType {
                id: killSwitchButton
                visible: !GC.isMobile()

                Layout.fillWidth: true

                text: qsTr("KillSwitch")
                descriptionText: qsTr("Блокирует сеть при отключении VPN")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsKillSwitch)
                }
            }

            DividerType {
                visible: GC.isDesktop()
            }
        }
    }
}
