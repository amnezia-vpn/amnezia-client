import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    FlickableType {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            anchors.fill: parent
            height: root.height

            Image {
                id: image
                source: "qrc:/images/zlovpn.svg"

                // antialiasing
                sourceSize.width: width
                sourceSize.height: height
                smooth: true
                antialiasing: true

                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                Layout.topMargin: 64
                Layout.preferredWidth: 300
                Layout.preferredHeight: 240
            }

            ColumnLayout {
                id: lookingForServer

                Layout.alignment: Qt.AlignHCenter
                visible: !AuthController.spikeErrored
                Header2Type {
                    headerText: qsTr("Looking for server")
                    Layout.alignment: Qt.AlignHCenter
                }

                ParagraphTextType {
                    Layout.alignment: Qt.AlignHCenter

                    text: qsTr("Please wait")
                    color: AmneziaStyle.color.mutedGray
                }

                SpinnerType {}
            }

            ColumnLayout {
                id: error

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16
                visible: AuthController.spikeErrored

                Header2Type {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 4

                    headerText: qsTr("Failed to find server")
                }

                ParagraphTextType {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 10

                    font.pixelSize: 20

                    text: qsTr("Possible reasons:")
                }

                ColumnLayout {
                    Layout.bottomMargin: 32

                    ParagraphTextType {
                        Layout.alignment: Qt.AlignLeft
                        Layout.maximumWidth: parent.width
                        text: qsTr("• You are having connection issues")
                    }
                    ParagraphTextType {
                        Layout.alignment: Qt.AlignLeft
                        Layout.maximumWidth: parent.width
                        text: qsTr("• One of our servers has been blocked")
                    }
                    ParagraphTextType {
                        Layout.alignment: Qt.AlignLeft
                        Layout.maximumWidth: parent.width
                        text: qsTr("• Our servers are down")
                    }
                    ParagraphTextType {
                        Layout.alignment: Qt.AlignLeft
                        Layout.maximumWidth: parent.width
                        text: qsTr("• Your app is out of date")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom

                    BasicButtonType {
                        id: websiteButton
                        Layout.fillWidth: true

                        text: qsTr("Website")
                        imageSource: "qrc:/images/controls/browser.svg"

                        clickedFunc: function() {
                            Qt.openUrlExternally(LanguageModel.getZloVpnSiteUrl())
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        BasicButtonType {
                            id: telegramButton
                            text: qsTr("Telegram")
                            imageSource: "qrc:/images/controls/telegram.svg"

                            Layout.preferredWidth: parent.width / 2
                            Layout.fillWidth: true

                            clickedFunc: function() {
                                Qt.openUrlExternally("https://t.me/zlovpn")
                            }
                        }

                        BasicButtonType {
                            id: mailButton
                            text: qsTr("Email")
                            imageSource: "qrc:/images/controls/mail.svg"

                            Layout.preferredWidth: parent.width / 2
                            Layout.fillWidth: true

                            clickedFunc: function() {
                                GC.copyToClipBoard("support@zlovpn.com")
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: AuthController

        function onSpikeUpdated() {
            print("spike updated")
            if (AuthController.spikeReady) {
                PageController.goToPageHome()
            }
        }

        function onSpikeErrorOccurred() {
            root.lookingForServer.visible = false
            root.error.visible = true
        }
    }

    Component.onCompleted: {
        if (AuthController.spikeReady) {
            PageController.goToPageHome()
        }
    }
}
