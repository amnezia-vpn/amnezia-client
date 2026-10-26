pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    property bool isRenewalAvailable: false

    expandedHeight: content.implicitHeight + 8 + PageController.safeAreaBottomMargin

    onAboutToShow: {
        isRenewalAvailable = ServersUiController.isServerRenewalAvailable(ServersUiController.defaultServerId) && !ApiAccountInfoModel.data("isInAppPurchase")
    }

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        SectionHeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            headerText: ServersUiController.serverName(ServersUiController.defaultServerId) + qsTr(" subscription has expired")
            descriptionText: root.isRenewalAvailable ? qsTr("Renew to continue using VPN") : ""
        }

        ButtonType {
            visible: root.isRenewalAvailable

            Layout.fillWidth: true
            Layout.topMargin: 32
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            text: qsTr("Renew")

            onClicked: {
                SubscriptionUiController.getRenewalLink(ServersUiController.defaultServerId)
            }
        }

        ButtonType {
            Layout.fillWidth: true
            Layout.topMargin: root.isRenewalAvailable ? 12 : 32
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            size: ButtonType.Medium
            variant: ButtonType.Text

            text: qsTr("Support")

            onClicked: {
                PageController.showBusyIndicator(true)
                let result = SubscriptionUiController.getAccountInfo(ServersUiController.defaultServerId, false)
                PageController.showBusyIndicator(false)
                if (result) {
                    root.close()
                    PageController.goToPage(PageEnum.PageSettingsApiSupport)
                }
            }
        }
    }
}
