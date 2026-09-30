import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType2 {
    id: root

    expandedStateContent: Item {
        id: contentRoot

        implicitHeight: content.implicitHeight + 40 + PageController.safeAreaBottomMargin

        Binding {
            target: root
            property: "expandedHeight"
            value: contentRoot.implicitHeight
        }

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true

                text: qsTr("Support")
            }

            AppTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8

                color: AmneziaStyle.color.textTertiary
                text: qsTr("If the update won't install, message us")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Telegram")
                descriptionText: qsTr("We'll reply in chat")
                leftImageSource: "qrc:/images/controls/telegram.svg"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("https://t.me/amnezia_vpn_en"))
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("support@amnezia.org")
                descriptionText: qsTr("Support email")
                leftImageSource: "qrc:/images/controls/mail.svg"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("mailto:support@amnezia.org"))
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("amnezia.org")
                descriptionText: qsTr("Download the update manually")
                leftImageSource: "qrc:/images/controls/amnezia.svg"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl(""))
                }
            }
        }
    }
}
