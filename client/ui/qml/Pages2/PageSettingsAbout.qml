import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
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
                color: "#CCCAC8"
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Contacts")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Telegram group")
                descriptionText: qsTr("To discuss features")
                leftImageSource: "qrc:/images/controls/telegram.svg"

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("https://t.me/amnezia_vpn_en"))
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Mail")
                descriptionText: qsTr("For reviews and bug reports")
                leftImageSource: "qrc:/images/controls/mail.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Github")
                leftImageSource: "qrc:/images/controls/github.svg"

                clickedFunction: function() {
                    Qt.openUrlExternally(qsTr("https://github.com/amnezia-vpn/amnezia-client"))
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Website")
                leftImageSource: "qrc:/images/controls/amnezia.svg"

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
                color: "#878B91"
            }

            BasicButtonType {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#FBB26A"

                text: qsTr("Check for updates")

                clickedFunc: function() {
                    Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
                }
            }

            BasicButtonType {
              Layout.alignment: Qt.AlignHCenter
              Layout.bottomMargin: 16
              Layout.topMargin: -15
              implicitHeight: 25

              defaultColor: "transparent"
              hoveredColor: Qt.rgba(1, 1, 1, 0.08)
              pressedColor: Qt.rgba(1, 1, 1, 0.12)
              disabledColor: "#878B91"
              textColor: "#FBB26A"

              text: qsTr("Privay Policy")

              clickedFunc: function() {
                Qt.openUrlExternally("https://amnezia.org/en/policy")
              }
            }
        }
    }
}
