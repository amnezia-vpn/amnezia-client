pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    property bool isRenewalAvailable: false

    onOpened: {
        isRenewalAvailable = ServersUiController.isServerRenewalAvailable(ServersUiController.defaultServerId) && !ApiAccountInfoModel.data("isInAppPurchase")
    }

    expandedStateContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        onImplicitHeightChanged: {
            root.expandedHeight = content.implicitHeight + 32 + PageController.safeAreaBottomMargin
        }

        Item {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            implicitHeight: titleText.implicitHeight

            Header2TextType {
                id: titleText
                anchors.left: parent.left
                anchors.right: parent.right

                text: ServersUiController.serverName(ServersUiController.defaultServerId) + qsTr(" subscription has expired")
                horizontalAlignment: Text.AlignLeft
            }
        }

        ParagraphTextType {
            visible: root.isRenewalAvailable

            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            text: qsTr("Renew to continue using VPN")
            horizontalAlignment: Text.AlignLeft
        }

        BasicButtonType {
            visible: root.isRenewalAvailable

            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            text: qsTr("Renew")

            defaultColor: AmneziaStyle.color.paleGray
            hoveredColor: AmneziaStyle.color.lightGray
            pressedColor: AmneziaStyle.color.mutedGray
            textColor: AmneziaStyle.color.midnightBlack

            clickedFunc: function() {
                SubscriptionUiController.getRenewalLink(ServersUiController.defaultServerId)
            }
        }

        BasicButtonType {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            Layout.bottomMargin: 8

            implicitHeight: 25

            defaultColor: AmneziaStyle.color.transparent
            hoveredColor: AmneziaStyle.color.translucentWhite
            pressedColor: AmneziaStyle.color.sheerWhite
            textColor: AmneziaStyle.color.goldenApricot

            text: qsTr("Support")

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                let result = SubscriptionUiController.getAccountInfo(ServersUiController.defaultServerId, false)
                PageController.showBusyIndicator(false)
                if (result) {
                    root.closeTriggered()
                    PageController.goToPage(PageEnum.PageSettingsApiSupport)
                }
            }
        }
    }
}
