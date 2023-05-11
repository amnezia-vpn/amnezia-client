import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"

PageBase {
    id: root
    page: PageEnum.Test
    logic: ViewConfigLogic

    ColumnLayout {
        id: content
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        HeaderType {
            id: header

            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.topMargin: 20
            Layout.bottomMargin: 32
            Layout.fillWidth: true

            backButtonImage: "qrc:/images/controls/arrow-left.svg"
            headerText: "Server 1"
            descriptionText: "root 192.168.111.111"
        }

        Item {
            Layout.fillWidth: true

            TabBar {
                id: tabBar

                anchors {
                    top: parent.top
                    right: parent.right
                    left: parent.left
                }

                background: Rectangle {
                    color: "transparent"
                }

                TabButtonType {
                    id: bb
                    isSelected: tabBar.currentIndex === 0
                    text: qsTr("Протоколы")
                }
                TabButtonType {
                    isSelected: tabBar.currentIndex === 1
                    text: qsTr("Сервисы")
                }
                TabButtonType {
                    isSelected: tabBar.currentIndex === 2
                    text: qsTr("Данные")
                }
            }

            StackLayout {
                id: stackLayout
                currentIndex: tabBar.currentIndex

                anchors.top: tabBar.bottom
                anchors.topMargin: 16

                width: parent.width
                height: root.height - header.implicitHeight - tabBar.implicitHeight - 100

                Item {
                    id: protocolsTab

                    FlickableType {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        contentHeight: protocolsTabContent.height

                        ColumnLayout {
                            id: protocolsTabContent
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right

                            BasicButtonType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                text: qsTr("Forget this server")
                            }

                            BasicButtonType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                defaultColor: "transparent"
                                hoveredColor: Qt.rgba(255, 255, 255, 0.08)
                                pressedColor: Qt.rgba(255, 255, 255, 0.12)
                                disabledColor: "#878B91"
                                textColor: "#D7D8DB"
                                borderWidth: 1

                                text: qsTr("Forget this server")
                            }

                            TextFieldWithHeaderType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                headerText: "Server IP adress [:port]"
                            }

                            LabelWithButtonType {
                                id: ip
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                text: "IP, логин и пароль от сервера"
                                buttonImage: "qrc:/images/controls/chevron-right.svg"
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                height: 1
                                color: "#2C2D30"
                            }

                            LabelWithButtonType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                text: "QR-код, ключ или файл настроек"
                                buttonImage: "qrc:/images/controls/chevron-right.svg"
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                height: 1
                                color: "#2C2D30"
                            }

                            CardType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                headerText: "Высокий"
                                bodyText: "Многие иностранные сайты и VPN-провайдеры заблокированы"
                                footerText: "футер"
                            }

                            CardType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                headerText: "Высокий"
                                bodyText: "Многие иностранные сайты и VPN-провайдеры заблокированы"
                                footerText: "футер"
                            }

                            DropDownType {
                                Layout.fillWidth: true

                                text: "IP, логин и пароль от сервера"
                                descriptionText: "IP, логин и пароль от сервера"

                                menuModel: [
                                    qsTr("SHA512"),
                                    qsTr("SHA384"),
                                    qsTr("SHA256"),
                                    qsTr("SHA3-512"),
                                    qsTr("SHA3-384"),
                                    qsTr("SHA3-256"),
                                    qsTr("whirlpool"),
                                    qsTr("BLAKE2b512"),
                                    qsTr("BLAKE2s256"),
                                    qsTr("SHA1")
                                ]
                            }
                        }
                    }
                }

                Item {
                    id: servicesTab

                    FlickableType {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        contentHeight: servicesTabContent.height

                        ColumnLayout {
                            id: servicesTabContent
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right

                            CheckBoxType {
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                text: qsTr("Auto-negotiate encryption")
                            }
                            CheckBoxType {
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.fillWidth: true
                                text: qsTr("Auto-negotiate encryption")
                                descriptionText: qsTr("Auto-negotiate encryption")
                            }

                            Rectangle {
                                implicitWidth: buttonGroup.implicitWidth
                                implicitHeight: buttonGroup.implicitHeight

                                Layout.leftMargin: 16
                                Layout.rightMargin: 16

                                color: "#1C1D21"
                                radius: 16

                                RowLayout {
                                    id: buttonGroup

                                    spacing: 0

                                    HorizontalRadioButton {
                                        implicitWidth: (root.width - 32) / 2
                                        text: "UDP"
                                    }

                                    HorizontalRadioButton {
                                        implicitWidth: (root.width - 32) / 2
                                        text: "TCP"
                                    }
                                }
                            }

                            VerticalRadioButton {
                                text: "Раздельное туннелирование"
                                descriptionText: "Позволяет подключаться к одним сайтам через защищенное соединение, а к другим в обход него"
                                checked: true

                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                            }

                            VerticalRadioButton {
                                text: "Раздельное туннелирование"

                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                            }

                            SwitcherType {
                                text: "Auto-negotiate encryption"

                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                            }
                        }
                    }
                }
            }
        }
    }
}
