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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    QtObject {
        id: telegramGroup

        readonly property string title: qsTr("Telegram group")
        readonly property string description: qsTr("To discuss features")
        readonly property string imageSource: "qrc:/images/controls/telegram.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally(qsTr("https://t.me/amnezia_vpn_en"))
        }
    }

    QtObject {
        id: mail

        readonly property string title: qsTr("support@amnezia.org")
        readonly property string description: qsTr("For reviews and bug reports")
        readonly property string imageSource: "qrc:/images/controls/mail.svg"
        readonly property var handler: function() {
            GC.copyToClipBoard(title)
            PageController.showNotificationMessage(qsTr("Copied"))
        }
    }

    QtObject {
        id: github

        readonly property string title: qsTr("GitHub")
        readonly property string description: qsTr("Discover the source code")
        readonly property string imageSource: "qrc:/images/controls/github.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally(qsTr("https://github.com/amnezia-vpn/amnezia-client"))
        }
    }

    QtObject {
        id: website

        readonly property string title: qsTr("Website")
        readonly property string description: qsTr("Visit official website")
        readonly property string imageSource: "qrc:/images/controls/amnezia.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl())
        }
    }

    property list<QtObject> contacts: [
        telegramGroup,
        mail,
        github,
        website
    ]

    ListView {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property bool isFocusable: true

        Keys.onTabPressed: {
            FocusController.nextKeyTabItem()
        }

        Keys.onBacktabPressed: {
            FocusController.previousKeyTabItem()
        }

        Keys.onUpPressed: {
            FocusController.nextKeyUpItem()
        }

        Keys.onDownPressed: {
            FocusController.nextKeyDownItem()
        }

        Keys.onLeftPressed: {
            FocusController.nextKeyLeftItem()
        }

        Keys.onRightPressed: {
            FocusController.nextKeyRightItem()
        }

        ScrollBar.vertical: ScrollBarType {}

        model: contacts

        clip: true

        header: ColumnLayout {
            width: listView.width

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
                color: AmneziaStyle.color.paleGray
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Contacts")
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                id: telegramButton
                Layout.fillWidth: true
                Layout.topMargin: 6

                text: title
                descriptionText: description
                leftImageSource: imageSource

                clickedFunction: handler
            }

            DividerType {}

        }

        footer: ColumnLayout {
            width: listView.width

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
                id: checkUpdatesButton

                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Check for updates")

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
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Privacy Policy")

                clickedFunc: function() {
                    Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl() + "/policy")
                }
            }
        }
    }
}
