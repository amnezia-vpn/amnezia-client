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
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            // FBLink VPN Brand Header
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                height: 120
                radius: 20
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#005A80" }
                    GradientStop { position: 1.0; color: "#00C8FF" }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    // Logo
                    Rectangle {
                        width: 60
                        height: 60
                        radius: 14
                        color: "transparent"
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: "qrc:/images/fblink_logo.png"
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: 60
                            sourceSize.height: 60
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: "FBLink VPN"
                            font.pixelSize: 24
                            font.weight: Font.Bold
                            color: "#FFFFFF"
                        }

                        Text {
                            text: qsTr("Надёжная защита в интернете")
                            font.pixelSize: 13
                            color: Qt.rgba(1, 1, 1, 0.75)
                        }
                    }
                }
            }

            // Version
            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Версия %1").arg(SettingsController.getAppVersion())
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

            // About text
            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
                wrapMode: Text.WordWrap

                text: qsTr("FBLink VPN — коммерческий VPN-сервис, созданный для безопасного и приватного доступа в интернет. Мы обеспечиваем шифрование трафика и скрытие вашего реального IP-адреса.\n\nПриложение создано на основе открытого проекта AmneziaVPN (amnezia-vpn.org), распространяемого по лицензии GNU GPL v3.")
                color: AmneziaStyle.color.paleGray
            }

            // Divider
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                height: 1
                color: "#2A2A30"
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Контакты и поддержка")
                font.weight: Font.Medium
            }
        }

        model: contacts

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
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

            // ── Account section ───────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 8
                visible: FBLinkController.isLoggedIn

                // Account info
                Rectangle {
                    Layout.fillWidth: true
                    height: accountRow.implicitHeight + 20
                    radius: 12
                    color: Qt.rgba(0, 200/255, 255/255, 0.07)
                    border.color: Qt.rgba(0, 200/255, 255/255, 0.2)
                    border.width: 1

                    RowLayout {
                        id: accountRow
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                        anchors.leftMargin: 16; anchors.rightMargin: 16
                        spacing: 10

                        Rectangle {
                            width: 32; height: 32; radius: 16
                            color: Qt.rgba(0, 200/255, 255/255, 0.15)
                            Text { anchors.centerIn: parent; text: "👤"; font.pixelSize: 15 }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: FBLinkController.isSubscribed
                                    ? qsTr("Premium аккаунт")
                                    : qsTr("Бесплатный аккаунт")
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                color: "#FFFFFF"
                            }

                            Text {
                                visible: FBLinkController.isSubscribed
                                text: qsTr("Действует до: ") + FBLinkController.subscriptionEndDate
                                font.pixelSize: 11
                                color: Qt.rgba(1, 1, 1, 0.5)
                            }
                        }
                    }
                }

                // Logout button
                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    radius: 12
                    color: logoutMouse.pressed
                        ? Qt.rgba(239/255, 68/255, 68/255, 0.25)
                        : (logoutMouse.containsMouse
                            ? Qt.rgba(239/255, 68/255, 68/255, 0.15)
                            : Qt.rgba(239/255, 68/255, 68/255, 0.08))
                    border.color: Qt.rgba(239/255, 68/255, 68/255, 0.4)
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 120 } }

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "→"
                            font.pixelSize: 16
                            color: "#EF4444"
                        }

                        Text {
                            text: qsTr("Выйти из аккаунта")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: "#EF4444"
                        }
                    }

                    MouseArea {
                        id: logoutMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            FBLinkController.logout()
                            PageController.goToPageHome()
                        }
                    }
                }
            }

            // Open source notice
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32
                height: openSourceText.implicitHeight + 24
                radius: 12
                color: "#1C1C22"
                border.color: "#333340"
                border.width: 1

                Text {
                    id: openSourceText
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        margins: 12
                    }
                    text: qsTr("🏛 Основано на AmneziaVPN (GNU GPL v3)\nИсходный код: github.com/amnezia-vpn")
                    font.pixelSize: 12
                    color: "#8A8A8E"
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://github.com/amnezia-vpn/amnezia-client")
                    }
                }
            }
        }
    }

    property list<QtObject> contacts: [
        telegramSupport,
        mailSupport
    ]

    QtObject {
        id: telegramSupport

        readonly property string title: qsTr("Telegram поддержка")
        readonly property string description: qsTr("Задайте вопрос нашей команде")
        readonly property string imageSource: "qrc:/images/controls/telegram.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally("https://t.me/fblink_vpn")
        }
    }

    QtObject {
        id: mailSupport

        readonly property string title: qsTr("support@fblink.com")
        readonly property string description: qsTr("По вопросам и жалобам")
        readonly property string imageSource: "qrc:/images/controls/mail.svg"
        readonly property var handler: function() {
            Qt.openUrlExternally("mailto:support@fblink.com")
        }
    }
}
