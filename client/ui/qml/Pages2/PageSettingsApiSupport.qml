import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Support")
                description: qsTr("Our technical support specialists are available to assist you at any time")
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Telegram")
                subtitle: "@" + ApiAccountInfoModel.getTelegramBotLink()
                trailingIconSource: "qrc:/images/controls/external-link.svg"

                onClicked: Qt.openUrlExternally("https://telegram.me/" + ApiAccountInfoModel.getTelegramBotLink())
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Email")
                subtitle: ApiAccountInfoModel.getEmailLink()
                trailingIconSource: "qrc:/images/controls/external-link.svg"

                onClicked: Qt.openUrlExternally("mailto:" + ApiAccountInfoModel.getEmailLink())
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Email Billing & Orders")
                subtitle: ApiAccountInfoModel.getBillingEmailLink()
                trailingIconSource: "qrc:/images/controls/external-link.svg"

                onClicked: Qt.openUrlExternally("mailto:" + ApiAccountInfoModel.getBillingEmailLink())
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ApiAccountInfoModel.getFullSiteLink() !== ""

                text: qsTr("Website")
                subtitle: ApiAccountInfoModel.getSiteLink()
                trailingIconSource: "qrc:/images/controls/external-link.svg"

                onClicked: Qt.openUrlExternally(ApiAccountInfoModel.getFullSiteLink())
            }

            ListItemType {
                id: supportUuid
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                showSeparator: false

                overline: qsTr("Support tag")
                text: SettingsController.getInstallationUuid()

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }
        }
    }
}
