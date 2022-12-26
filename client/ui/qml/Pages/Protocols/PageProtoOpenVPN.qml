import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.OpenVpn
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("OpenVPN Settings")
    }

    FlickableType {
        id: fl
        width: root.width
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.left: root.left
        anchors.leftMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 15

        contentHeight: content.height
        clip: true

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 15

            LabelType {
                id: lb_subnet
                height: 21
                text: qsTr("VPN Addresses Subnet")
            }
            TextFieldType {
                id: tf_subnet

                implicitWidth: parent.width
                height: 31
                text: logic.lineEditSubnetText
                onEditingFinished: {
                    logic.lineEditSubnetText = text
                }
            }

            //
            LabelType {
                id: lb_proto
                Layout.topMargin: 20
                height: 21
                text: qsTr("Network protocol")
            }
            Rectangle {
                id: rect_proto
                implicitWidth: root.width - 60
                height: 71
                border.width: 1
                border.color: "lightgray"
                radius: 2
                RadioButtonType {
                    x: 10
                    y: 40
                    width: 171
                    height: 19
                    text: qsTr("TCP")
                    enabled: logic.radioButtonTcpEnabled
                    checked: logic.radioButtonTcpChecked
                    onCheckedChanged: {
                        UiLogic.radioButtonTcpChecked = checked
                    }
                }
                RadioButtonType {
                    x: 10
                    y: 10
                    width: 171
                    height: 19
                    text: qsTr("UDP")
                    checked: logic.radioButtonUdpChecked
                    onCheckedChanged: {
                        logic.radioButtonUdpChecked = checked
                    }
                    enabled: logic.radioButtonUdpEnabled
                }
            }

            //
            RowLayout {
                Layout.topMargin: 10
                Layout.fillWidth: true
                LabelType {
                    id: lb_port
                    height: 31
                    text: qsTr("Port")
                    Layout.preferredWidth: root.width / 2 - 10
                }
                TextFieldType {
                    id: tf_port
                    Layout.fillWidth: true

                    height: 31
                    text: logic.lineEditPortText
                    onEditingFinished: {
                        logic.lineEditPortText = text
                    }
                    enabled: logic.lineEditPortEnabled
                }
            }



            //
            CheckBoxType {
                id: check_auto_enc

                implicitWidth: parent.width
                height: 21
                text: qsTr("Auto-negotiate encryption")
                checked: logic.checkBoxAutoEncryptionChecked
                onCheckedChanged: {
                    logic.checkBoxAutoEncryptionChecked = checked
                }
                onClicked: {
                    logic.checkBoxAutoEncryptionClicked()
                }
            }

            //
            LabelType {
                id: lb_cipher
                height: 21
                text: qsTr("Cipher")
            }

            ComboBoxType {
                id: cb_cipher
                implicitWidth: parent.width

                height: 31
                model: [
                    qsTr("AES-256-GCM"),
                    qsTr("AES-192-GCM"),
                    qsTr("AES-128-GCM"),
                    qsTr("AES-256-CBC"),
                    qsTr("AES-192-CBC"),
                    qsTr("AES-128-CBC"),
                    qsTr("ChaCha20-Poly1305"),
                    qsTr("ARIA-256-CBC"),
                    qsTr("CAMELLIA-256-CBC"),
                    qsTr("none")
                ]
                currentIndex: {
                    for (let i = 0; i < model.length; ++i) {
                        if (logic.comboBoxVpnCipherText === model[i]) {
                            return i
                        }
                    }
                    return -1
                }
                onCurrentTextChanged: {
                    logic.comboBoxVpnCipherText = currentText
                }
                enabled: !check_auto_enc.checked
            }

            //
            LabelType {
                id: lb_hash
                height: 21
                Layout.topMargin: 20
                text: qsTr("Hash")
            }
            ComboBoxType {
                id: cb_hash
                height: 31
                implicitWidth: parent.width
                model: [
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
                currentIndex: {
                    for (let i = 0; i < model.length; ++i) {
                        if (logic.comboBoxVpnHashText === model[i]) {
                            return i
                        }
                    }
                    return -1
                }
                onCurrentTextChanged: {
                    logic.comboBoxVpnHashText = currentText
                }
                enabled: !check_auto_enc.checked
            }

            CheckBoxType {
                id: check_tls
                implicitWidth: parent.width
                Layout.topMargin: 20
                height: 21
                text: qsTr("Enable TLS auth")
                checked: logic.checkBoxTlsAuthChecked
                onCheckedChanged: {
                    logic.checkBoxTlsAuthChecked = checked
                }

            }

            CheckBoxType {
                id: check_block_dns
                implicitWidth: parent.width
                height: 21
                text: qsTr("Block DNS requests outside of VPN")
                checked: logic.checkBoxBlockDnsChecked
                onCheckedChanged: {
                    logic.checkBoxBlockDnsChecked = checked
                }
            }


            BasicButtonType {
                id: pb_client_config

                implicitWidth: parent.width
                height: 21
                text: qsTr("Additional client config commands →")
                background: Item {
                    anchors.fill: parent
                }

                contentItem: Text {
                    anchors.fill: parent
                    font.family: "Lato"
                    font.styleName: "normal"
                    font.pixelSize: 16
                    color: "#15CDCB";
                    text: pb_client_config.text
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                antialiasing: true
                checkable: true
                checked: StartPageLogic.pushButtonConnectKeyChecked
            }

            Rectangle {
                id: rect_client_conf
                implicitWidth: root.width - 60
                height: 101
                border.width: 1
                border.color: "lightgray"
                radius: 2
                visible: pb_client_config.checked

                ScrollView {
                    anchors.fill: parent
                    TextArea {
                        id: te_client_config
                        font.family: "Lato"
                        font.styleName: "normal"
                        font.pixelSize: 16
                        color: "#181922"
                        text: logic.textAreaAdditionalClientConfig
                        onEditingFinished: {
                            logic.textAreaAdditionalClientConfig = text
                        }
                    }
                }


            }


            BasicButtonType {
                id: pb_server_config

                implicitWidth: parent.width
                height: 21
                text: qsTr("Additional server config commands →")
                background: Item {
                    anchors.fill: parent
                }

                contentItem: Text {
                    anchors.fill: parent
                    font.family: "Lato"
                    font.styleName: "normal"
                    font.pixelSize: 16
                    color: "#15CDCB";
                    text: pb_server_config.text
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                antialiasing: true
                checkable: true
                checked: StartPageLogic.pushButtonConnectKeyChecked
            }

            Rectangle {
                id: rect_server_conf
                implicitWidth: root.width - 60
                height: 101
                border.width: 1
                border.color: "lightgray"
                radius: 2
                visible: pb_server_config.checked

                ScrollView {
                    anchors.fill: parent
                    TextArea {
                        id: te_server_config
                        font.family: "Lato"
                        font.styleName: "normal"
                        font.pixelSize: 16
                        color: "#181922"
                        text: logic.textAreaAdditionalServerConfig
                        onEditingFinished: {
                            logic.textAreaAdditionalServerConfig = text
                        }
                    }
                }


            }

            LabelType {
                id: label_proto_openvpn_info

                height: 41
                visible: logic.labelProtoOpenVpnInfoVisible
                text: logic.labelProtoOpenVpnInfoText
            }

            Rectangle {
                id: it_save
                implicitWidth: parent.width
                Layout.topMargin: 20
                height: 40

                BlueButtonType {
                    id: pb_save
                    z: 1
                    height: 40
                    text: qsTr("Save and restart VPN")
                    width: parent.width
                    visible: logic.pushButtonSaveVisible
                    onClicked: {
                        logic.onPushButtonProtoOpenVpnSaveClicked()
                    }
                }

                ProgressBar {
                    id: progress_save
                    anchors.fill: pb_save
                    from: 0
                    to: logic.progressBarResetMaximium
                    value: logic.progressBarResetValue
                    visible: logic.progressBarResetVisible
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
                            width: progress_save.visualPosition * parent.width
                            height: parent.height
                            radius: 4
                            color: Qt.rgba(255, 255, 255, 0.15);
                        }
                    }
                }

            }

        }

    }

}
