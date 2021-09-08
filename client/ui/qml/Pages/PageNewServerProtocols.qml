import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    BackButton {
        id: back
    }
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 24
        color: "#100A44"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Select VPN protocols")
        x: 10
        y: 35
        width: 361
        height: 31
    }
    ProgressBar {
        id: progress_bar_new_server_connection
        anchors.horizontalCenter: parent.horizontalCenter
        y: 570
        width: 301
        height: 40
        from: NewServerProtocolsLogic.progressBarConnectionMinimum
        to: NewServerProtocolsLogic.progressBarConnectionMaximum
        value: 0
        background: Rectangle {
            implicitWidth: parent.width
            implicitHeight: parent.height
            color: "#100A44"
            radius: 4
        }

        contentItem: Item {
            implicitWidth: parent.width
            implicitHeight: parent.height
            Rectangle {
                width: progress_bar_new_server_connection.visualPosition * parent.width
                height: parent.height
                radius: 4
                color: Qt.rgba(255, 255, 255, 0.15);
            }
        }

        LabelType {
            anchors.fill: parent
            text: qsTr("Configuring...")
            horizontalAlignment: Text.AlignHCenter
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#D4D4D4"
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 570
        width: 301
        height: 40
        text: qsTr("Setup server")
        onClicked: {
            NewServerProtocolsLogic.pushButtonConnectConfigureClicked()
        }
    }
    ScrollView {
        x: 0
        y: 70
        width: 380
        height: 471
        clip: true
        Column {
            spacing: 5
            Rectangle {
                x: 5
                y: 5
                width: 368
                height: frame_new_server_setting_cloak.visible ? 140 : 72
                border.width: 1
                border.color: "lightgray"
                radius: 2
                Rectangle {
                    id: frame_new_server_setting_cloak
                    height: 77
                    width: 353
                    border.width: 1
                    border.color: "lightgray"
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: false
                    radius: 2
                    Grid {
                        anchors.fill: parent
                        columns: 2
                        horizontalItemAlignment: Grid.AlignHCenter
                        verticalItemAlignment: Grid.AlignVCenter
                        topPadding: 5
                        leftPadding: 10
                        spacing: 5
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: qsTr("Port (TCP)")
                        }
                        TextFieldType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: NewServerProtocolsLogic.lineEditCloakPortText
                            onEditingFinished: {
                                NewServerProtocolsLogic.lineEditCloakPortText = text
                            }
                        }
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: qsTr("Fake Web Site")
                        }
                        TextFieldType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: NewServerProtocolsLogic.lineEditCloakSiteText
                            onEditingFinished: {
                                NewServerProtocolsLogic.lineEditCloakSiteText = text
                            }
                        }
                    }
                }
                Row {
                    anchors.top: parent.top
                    anchors.topMargin: 5
                    leftPadding: 15
                    rightPadding: 5
                    height: 55
                    width: parent.width
                    CheckBoxType {
                        text: qsTr("OpenVPN and ShadowSocks\n with masking using Cloak plugin")
                        height: parent.height
                        width: 308
                        checked: NewServerProtocolsLogic.checkBoxCloakChecked
                        onCheckedChanged: {
                            NewServerProtocolsLogic.checkBoxCloakChecked = checked
                        }
                    }
                    ImageButtonType {
                        width: 35
                        height: 35
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "qrc:/images/settings.png"
                        checkable: true
                        checked: NewServerProtocolsLogic.pushButtonSettingsCloakChecked
                        onCheckedChanged: {
                            NewServerProtocolsLogic.pushButtonSettingsCloakChecked = checked
                            if (checked) {
                                frame_new_server_setting_cloak.visible = true
                            } else {
                                frame_new_server_setting_cloak.visible = false
                            }
                        }
                    }
                }
            }
            Rectangle {
                x: 5
                y: 5
                width: 368
                height: frame_new_server_settings_ss.visible ? 140 : 72
                border.width: 1
                border.color: "lightgray"
                radius: 2
                Rectangle {
                    id: frame_new_server_settings_ss
                    height: 77
                    width: 353
                    border.width: 1
                    border.color: "lightgray"
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 2
                    visible: false
                    Grid {
                        anchors.fill: parent
                        columns: 2
                        horizontalItemAlignment: Grid.AlignHCenter
                        verticalItemAlignment: Grid.AlignVCenter
                        topPadding: 5
                        leftPadding: 10
                        spacing: 5
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: qsTr("Port (TCP)")
                        }
                        TextFieldType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: NewServerProtocolsLogic.lineEditSsPortText
                            onEditingFinished: {
                                NewServerProtocolsLogic.lineEditSsPortText = text
                            }
                        }
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: qsTr("Encryption")
                        }
                        ComboBoxType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            model: [
                                qsTr("chacha20-ietf-poly1305"),
                                qsTr("xchacha20-ietf-poly1305"),
                                qsTr("aes-256-gcm"),
                                qsTr("aes-192-gcm"),
                                qsTr("aes-128-gcm")
                            ]
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (NewServerProtocolsLogic.comboBoxSsCipherText === model[i]) {
                                        return i
                                    }
                                }
                                return -1
                            }
                            onCurrentTextChanged: {
                                NewServerProtocolsLogic.comboBoxSsCipherText = currentText
                            }
                        }
                    }
                }
                Row {
                    anchors.top: parent.top
                    anchors.topMargin: 5
                    leftPadding: 15
                    rightPadding: 5
                    height: 55
                    width: parent.width
                    CheckBoxType {
                        text: qsTr("ShadowSocks")
                        height: parent.height
                        width: 308
                        checked: NewServerProtocolsLogic.checkBoxSsChecked
                        onCheckedChanged:  {
                            NewServerProtocolsLogic.checkBoxSsChecked = checked
                        }
                    }
                    ImageButtonType {
                        width: 35
                        height: 35
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "qrc:/images/settings.png"
                        checked: NewServerProtocolsLogic.pushButtonSettingsSsChecked
                        checkable: true
                        onCheckedChanged: {
                            NewServerProtocolsLogic.pushButtonSettingsSsChecked = checked
                            if (checked) {
                                frame_new_server_settings_ss.visible = true
                            } else {
                                frame_new_server_settings_ss.visible = false
                            }
                        }
                    }
                }
            }
            Rectangle {
                x: 5
                y: 5
                width: 368
                height: frame_new_server_settings_openvpn.visible ? 140 : 72
                border.width: 1
                border.color: "lightgray"
                radius: 2
                Rectangle {
                    id: frame_new_server_settings_openvpn
                    height: 77
                    width: 353
                    border.width: 1
                    border.color: "lightgray"
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 2
                    visible: false
                    Grid {
                        anchors.fill: parent
                        columns: 2
                        horizontalItemAlignment: Grid.AlignHCenter
                        verticalItemAlignment: Grid.AlignVCenter
                        topPadding: 5
                        leftPadding: 10
                        spacing: 5
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: qsTr("Port (TCP)")
                        }
                        TextFieldType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: NewServerProtocolsLogic.lineEditOpenvpnPortText
                            onEditingFinished: {
                                NewServerProtocolsLogic.lineEditOpenvpnPortText = text
                            }
                        }
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            text: qsTr("Protocol")
                        }
                        ComboBoxType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2) / 2
                            model: [
                                qsTr("udp"),
                                qsTr("tcp"),
                            ]
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (NewServerProtocolsLogic.comboBoxOpenvpnProtoText === model[i]) {
                                        return i
                                    }
                                }
                                return -1
                            }
                            onCurrentTextChanged: {
                                NewServerProtocolsLogic.comboBoxOpenvpnProtoText = currentText
                            }
                        }
                    }
                }
                Row {
                    anchors.top: parent.top
                    anchors.topMargin: 5
                    leftPadding: 15
                    rightPadding: 5
                    height: 55
                    width: parent.width
                    CheckBoxType {
                        text: qsTr("OpenVPN")
                        height: parent.height
                        width: 308
                        checked: NewServerProtocolsLogic.checkBoxOpenvpnChecked
                        onCheckedChanged: {
                            NewServerProtocolsLogic.checkBoxOpenvpnChecked = checked
                        }
                    }
                    ImageButtonType {
                        width: 35
                        height: 35
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "qrc:/images/settings.png"
                        checked: NewServerProtocolsLogic.pushButtonSettingsOpenvpnChecked
                        checkable: true
                        onCheckedChanged: {
                            NewServerProtocolsLogic.pushButtonSettingsOpenvpnChecked = checked
                            if (checked) {
                                frame_new_server_settings_openvpn.visible = true
                            } else {
                                frame_new_server_settings_openvpn.visible = false
                            }
                        }
                    }
                }
            }
            Rectangle {
                id: frame_new_server_settings_parent_wireguard
                visible: NewServerProtocolsLogic.frameSettingsParentWireguardVisible
                x: 5
                y: 5
                width: 368
                height: frame_new_server_settings_wireguard.visible ? 109 : 72
                border.width: 1
                border.color: "lightgray"
                radius: 2
                Rectangle {
                    id: frame_new_server_settings_wireguard
                    height: 46
                    width: 353
                    border.width: 1
                    border.color: "lightgray"
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 2
                    visible: false
                    Grid {
                        anchors.fill: parent
                        columns: 2
                        horizontalItemAlignment: Grid.AlignHCenter
                        verticalItemAlignment: Grid.AlignVCenter
                        topPadding: 5
                        leftPadding: 10
                        spacing: 5
                        LabelType {
                            width: 130
                            height: (parent.height - parent.spacing - parent.topPadding * 2)
                            text: qsTr("Port (TCP)")
                        }
                        TextFieldType {
                            width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                            height: (parent.height - parent.spacing - parent.topPadding * 2)
                            text: "32767"
                        }
                    }
                }
                Row {
                    anchors.top: parent.top
                    anchors.topMargin: 5
                    leftPadding: 15
                    rightPadding: 5
                    height: 55
                    width: parent.width
                    CheckBoxType {
                        text: qsTr("WireGuard")
                        height: parent.height
                        width: 308
                    }
                    ImageButtonType {
                        width: 35
                        height: 35
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "qrc:/images/settings.png"
                        checkable: true
                        onCheckedChanged: {
                            if (checked) {
                                frame_new_server_settings_wireguard.visible = true
                            } else {
                                frame_new_server_settings_wireguard.visible = false
                            }
                        }
                    }
                }
            }
        }
    }
}
