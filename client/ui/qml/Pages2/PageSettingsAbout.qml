import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    Item {
        id: focusItem
        KeyNavigation.tab: backButton

        onFocusChanged: {
            if (focusItem.activeFocus) {
                fl.contentY = 0
            }
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        KeyNavigation.tab: mailButton
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Image {
                id: image
                source: "qrc:/images/zlovpn.svg"

                Layout.alignment: Qt.AlignCenter
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredWidth: 291
                Layout.preferredHeight: 224
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.maximumWidth: parent.width

                horizontalAlignment: Text.AlignHCenter

                height: 20
                font.pixelSize: 14

                text: qsTr("ZloVPN is an easy to use VPN application based on AmneziaVPN.")
                color: AmneziaStyle.color.paleGray
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Contacts")
            }

            LabelWithButtonType {
                id: mailButton
                Layout.fillWidth: true

                text: qsTr("Telegram")
                descriptionText: qsTr("For issues and suggestions")
                leftImageSource: "qrc:/images/controls/telegram.svg"

                KeyNavigation.tab: websiteButton
                parentFlickable: fl

                clickedFunction: function() {
                    Qt.openUrlExternally("https://t.me/zlovpn")
                }

            }

            DividerType {}

            LabelWithButtonType {
                id: websiteButton
                Layout.fillWidth: true

                text: qsTr("Website")
                leftImageSource: "qrc:/images/controls/browser.svg"

                KeyNavigation.tab: privacyPolicyButton
                parentFlickable: fl

                clickedFunction: function() {
                    Qt.openUrlExternally(LanguageModel.getZloVpnSiteUrl())
                }

            }

            DividerType {}

            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 40

                horizontalAlignment: Text.AlignHCenter

                text: qsTr("Software version: %1").arg(SettingsController.getAppVersion())
                color: AmneziaStyle.color.mutedGray

                MouseArea {
                    property int clickCount: 0
                    anchors.fill: parent
                    onClicked: {
                        if (clickCount > 10) {
                            SettingsController.enableDevMode()
                        } else {
                            clickCount++
                        }
                    }
                }
            }

            BasicButtonType {
              id: privacyPolicyButton
              Layout.alignment: Qt.AlignHCenter
              Layout.bottomMargin: 16
              implicitHeight: 25

              defaultColor: AmneziaStyle.color.transparent
              hoveredColor: AmneziaStyle.color.translucentWhite
              pressedColor: AmneziaStyle.color.sheerWhite
              disabledColor: AmneziaStyle.color.mutedGray
              textColor: AmneziaStyle.color.goldenApricot

              KeyNavigation.tab: aboutAmneziaButton
              text: qsTr("Privacy Policy")

              parentFlickable: fl

              clickedFunc: function() {
                Qt.openUrlExternally(LanguageModel.getZloVpnSiteUrl() + "/privacy-policy")
              }
            }

            BasicButtonType {
              id: aboutAmneziaButton
              Layout.alignment: Qt.AlignHCenter
              Layout.bottomMargin: 16
              Layout.topMargin: -12
              implicitHeight: 25

              defaultColor: AmneziaStyle.color.transparent
              hoveredColor: AmneziaStyle.color.translucentWhite
              pressedColor: AmneziaStyle.color.sheerWhite
              disabledColor: AmneziaStyle.color.mutedGray
              textColor: AmneziaStyle.color.goldenApricot

              text: qsTr("About AmneziaVPN")

              Keys.onTabPressed: lastItemTabClicked()
              parentFlickable: fl

              clickedFunc: function() {
                PageController.goToPage(PageEnum.PageSettingsAboutOriginal)
              }
            }
        }
    }
}
