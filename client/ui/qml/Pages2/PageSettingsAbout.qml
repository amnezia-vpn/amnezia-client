import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

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

            Image {
                id: image

                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 153
                Layout.preferredHeight: 131

                source: "qrc:/images/amneziaLogo.svg"
                sourceSize: Qt.size(153 * Screen.devicePixelRatio, 131 * Screen.devicePixelRatio)
                fillMode: Image.PreserveAspectFit
            }

            H1TextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Support Amnezia")
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                horizontalAlignment: Text.AlignHCenter
                color: AmneziaStyle.color.textSecondary
                text: qsTr("Amnezia is a free and open-source application. You can support the developers if you like it.")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    BodyTextType {
                        Layout.fillWidth: true

                        color: AmneziaStyle.color.textTertiary
                        text: qsTr("Contacts")
                    }

                    ColumnLayout {
                        Layout.fillWidth: true

                        spacing: 0

                        Repeater {
                            model: contacts

                            delegate: ListItemType {
                                Layout.fillWidth: true

                                text: title
                                subtitle: description
                                icon.source: imageSource

                                onClicked: handler()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: content.width - 32

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

                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: Math.max(220, implicitWidth)
                        Layout.maximumWidth: content.width - 32

                        variant: ButtonType.Secondary
                        loading: UpdateController.isCheckRunning
                        enabled: !UpdateController.isCheckRunning

                        text: UpdateController.isCheckRunning ? qsTr("Checking...") : qsTr("Check for updates")

                        onClicked: {
                            UpdateController.checkForUpdates()
                        }
                    }

                    ButtonType {
                        id: privacyPolicyButton

                        Layout.alignment: Qt.AlignHCenter

                        variant: ButtonType.Text
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
