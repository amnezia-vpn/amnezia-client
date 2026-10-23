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

    QtObject {
        id: telegram

        readonly property string title: qsTr("Telegram")
        readonly property string description: "@" + ApiAccountInfoModel.getTelegramBotLink()
        readonly property string link: "https://telegram.me/" + ApiAccountInfoModel.getTelegramBotLink()
    }

    QtObject {
        id: techSupport

        readonly property string title: qsTr("Email")
        readonly property string description: ApiAccountInfoModel.getEmailLink()
        readonly property string link: "mailto:" + ApiAccountInfoModel.getEmailLink()
    }

    QtObject {
        id: paymentSupport

        readonly property string title: qsTr("Email Billing & Orders")
        readonly property string description: ApiAccountInfoModel.getBillingEmailLink()
        readonly property string link: "mailto:" + ApiAccountInfoModel.getBillingEmailLink()
    }

    QtObject {
        id: site

        readonly property string title: qsTr("Website")
        readonly property string description: ApiAccountInfoModel.getSiteLink()
        readonly property string link: ApiAccountInfoModel.getFullSiteLink()
    }

    property list<QtObject> supportModel: [
        telegram,
        techSupport,
        paymentSupport,
        site
    ]

    ListViewType {
        id: listView

        anchors.fill: parent

        model: supportModel

        header: ColumnLayout {
            width: listView.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Support")
                description: qsTr("Our technical support specialists are available to assist you at any time")
            }

            Item {
                Layout.preferredHeight: 16
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: link !== ""
                text: title
                subtitle: description
                trailingIconSource: "qrc:/images/controls/external-link.svg"
                onClicked: {
                    Qt.openUrlExternally(link)
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

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
                    if (!GC.isMobile()) {
                        supportUuid.forceActiveFocus(Qt.TabFocusReason)
                    }
                }
            }
        }
    }
}
