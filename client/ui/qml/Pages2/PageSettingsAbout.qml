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

        KeyNavigation.tab: telegramButton
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
                source: "qrc:/images/amneziaBigLogo.png"

                Layout.alignment: Qt.AlignCenter
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredWidth: 291
                Layout.preferredHeight: 224
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Support Amnezia")
                horizontalAlignment: Text.AlignHCenter
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                horizontalAlignment: Text.AlignHCenter

                height: 20
                font.pixelSize: 14

                text: qsTr("Amnezia is a free and open-source application. You can support the developers if you like it.")
                color: AmneziaStyle.color.white
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Contacts")
            }

            LabelWithButtonType {
                id: telegramButton
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Telegram group")
                descriptionText: qsTr("To discuss features")
                leftImageSource: "qrc:/images/controls/telegram.svg"

                KeyNavigation.tab: mailButton
                parentFlickable: fl

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("https://t.me/amnezia_vpn_en"))
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: mailButton
                Layout.fillWidth: true

                text: qsTr("Mail")
                descriptionText: qsTr("For reviews and bug reports")
                leftImageSource: "qrc:/images/controls/mail.svg"

                KeyNavigation.tab: githubButton
                parentFlickable: fl

                clickedFunction: function() {
                }

            }

            DividerType {}

            LabelWithButtonType {
                id: githubButton
                Layout.fillWidth: true

                text: qsTr("GitHub")
                leftImageSource: "qrc:/images/controls/github.svg"

                KeyNavigation.tab: websiteButton
                parentFlickable: fl

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("https://github.com/amnezia-vpn/amnezia-client"))
                }

            }

            DividerType {}

            LabelWithButtonType {
                id: websiteButton
                Layout.fillWidth: true

                text: qsTr("Website")
                leftImageSource: "qrc:/images/controls/amnezia.svg"

                KeyNavigation.tab: checkUpdatesButton
                parentFlickable: fl

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("https://amnezia.org"))
                }

            }

            DividerType {}

            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 40

                horizontalAlignment: Text.AlignHCenter

                text: qsTr("Software version: %1").arg(SettingsController.getAppVersion())
                color: AmneziaStyle.color.grey

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
                id: checkUpdatesButton
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.blackHovered
                pressedColor: AmneziaStyle.color.blackPressed
                disabledColor: AmneziaStyle.color.grey
                textColor: AmneziaStyle.color.orange

                text: qsTr("Check for updates")

                KeyNavigation.tab: privacyPolicyButton
                parentFlickable: fl

                clickedFunc: function() {
                    Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
                }
            }

            BasicButtonType {
              id: privacyPolicyButton
              Layout.alignment: Qt.AlignHCenter
              Layout.bottomMargin: 16
              Layout.topMargin: -15
              implicitHeight: 25

              defaultColor: AmneziaStyle.color.transparent
              hoveredColor: AmneziaStyle.color.blackHovered
              pressedColor: AmneziaStyle.color.blackPressed
              disabledColor: AmneziaStyle.color.grey
              textColor: AmneziaStyle.color.orange

              text: qsTr("Privacy Policy")

              Keys.onTabPressed: lastItemTabClicked()
              parentFlickable: fl

              clickedFunc: function() {
                Qt.openUrlExternally("https://amnezia.org/en/policy")
              }
            }
        }
    }
}
