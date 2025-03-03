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
        readonly property string link: "https://t.me/" + ApiAccountInfoModel.getTelegramBotLink()
    }

    QtObject {
        id: techSupport

        readonly property string title: qsTr("Email Support")
        readonly property string description: qsTr("support@amnezia.org")
        readonly property string link: "mailto:support@amnezia.org"
    }

    QtObject {
        id: paymentSupport

        readonly property string title: qsTr("Email Billing & Orders")
        readonly property string description: qsTr("help@vpnpay.io")
        readonly property string link: "mailto:help@vpnpay.io"
    }

    QtObject {
        id: site

        readonly property string title: qsTr("Website")
        readonly property string description: qsTr("amnezia.org")
        readonly property string link: LanguageModel.getCurrentSiteUrl()
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
        anchors.topMargin: 20
        anchors.bottomMargin: 24

        model: supportModel

        header: ColumnLayout {
            width: listView.width

            BackButtonType {
                id: backButton
            }

            HeaderType {
                id: header

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Support")
                descriptionText: qsTr("Our technical support specialists are available to assist you at any time")
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true
                text: title
                descriptionText: description
                rightImageSource: "qrc:/images/controls/external-link.svg"
                clickedFunction: function() {
                    Qt.openUrlExternally(link)
                }
            }
            DividerType {}
        }


        footer: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                id: supportUuid
                Layout.fillWidth: true

                text: qsTr("Support tag")
                descriptionText: SettingsController.getInstallationUuid()

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                    if (!GC.isMobile()) {
                        this.rightButton.forceActiveFocus()
                    }
                }
            }
        }
    }
}
