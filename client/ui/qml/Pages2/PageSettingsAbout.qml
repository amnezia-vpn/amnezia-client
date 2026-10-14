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

    Connections {
        target: UpdateController

        function onUpdateNotFound() {
            PageController.showNotificationMessage(qsTr("You have the latest version of AmneziaVPN"))
        }

        function onUpdateCheckFailed() {
            PageController.showNotificationMessage(qsTr("Failed to check for updates"))
        }
    }

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
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    Image {
                        id: image

                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 291
                        Layout.preferredHeight: 224

                        source: "qrc:/images/amneziaBigLogo.png"
                    }

                    H2TextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12

                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Support Amnezia")
                    }

                    BodyTextType {
                        Layout.fillWidth: true

                        horizontalAlignment: Text.AlignHCenter
                        color: AmneziaStyle.color.textSecondary
                        text: qsTr("Amnezia is a free and open-source application. You can support the developers if you like it.")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    SmallTextType {
                        Layout.fillWidth: true

                        text: qsTr("Contacts")
                    }

                    ColumnLayout {
                        Layout.fillWidth: true

                        spacing: 0

                        Repeater {
                            model: contacts

                            delegate: ListItemType {
                                Layout.fillWidth: true

                                leadingSlotWidth: 24
                                text: title
                                subtitle: description
                                icon.source: imageSource
                                trailingIconSource: "qrc:/images/controls/external-link.svg"

                                onClicked: handler()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter

                        spacing: 4

                        CaptionTextType {
                            text: SettingsController.getAppVersion()

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

                        IconButtonType {
                            size: IconButtonType.Small
                            icon.source: "qrc:/images/controls/copy.svg"
                            Accessible.name: qsTr("Copy version")

                            onClicked: {
                                GC.copyToClipBoard(SettingsController.getAppVersion())
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }
                    }

                    ButtonType {
                        id: checkUpdatesButton

                        Layout.fillWidth: true

                        variant: ButtonType.Secondary
                        enabled: !UpdateController.isCheckRunning

                        text: UpdateController.isCheckRunning ? qsTr("Checking...") : qsTr("Check for updates")

                        onClicked: {
                            UpdateController.checkForUpdates()
                        }
                    }

                    LinkButtonType {
                        id: privacyPolicyButton

                        Layout.alignment: Qt.AlignHCenter

                        size: LinkButtonType.Medium
                        text: qsTr("Privacy Policy")

                        onClicked: {
                            Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl("policy"))
                        }
                    }
                }
            }
        }
    }

    property list<QtObject> contacts: [
        telegramGroup,
        mail,
        github,
        website
    ]

    QtObject {
        id: telegramGroup

        readonly property string title: qsTr("Telegram group")
        readonly property string description: qsTr("To discuss features")
        readonly property string imageSource: "qrc:/images/controls/telegram.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally(qsTr("https://telegram.me/amnezia_vpn_en"))
        }
    }

    QtObject {
        id: mail

        readonly property string title: qsTr("support@amnezia.org")
        readonly property string description: qsTr("For reviews and bug reports")
        readonly property string imageSource: "qrc:/images/controls/mail.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally(qsTr("mailto:support@amnezia.org"))
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
            Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl())
        }
    }
}
