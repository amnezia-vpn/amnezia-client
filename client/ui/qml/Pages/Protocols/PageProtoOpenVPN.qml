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

    Item {
        enabled: logic.pageEnabled
        anchors.top: caption.bottom
        anchors.bottom: parent.bottom
        width: parent.width

        LabelType {
            id: lb_subnet
            x: 30
            anchors.top: parent.top
            width: parent.width
            height: 21
            text: qsTr("VPN Addresses Subnet")
        }
        TextFieldType {
            id: tf_subnet
            x: 30
            anchors.top: lb_subnet.bottom

            width: parent.width - 60
            height: 31
            text: logic.lineEditProtoOpenVpnSubnetText
            onEditingFinished: {
                logic.lineEditProtoOpenVpnSubnetText = text
            }
        }

        //
        LabelType {
            id: lb_proto
            x: 30
            anchors.top: tf_subnet.bottom
            width: parent.width
            height: 21
            text: qsTr("Network protocol")
        }
        Rectangle {
            id: rect_proto
            x: 30
            anchors.top: lb_proto.bottom
            width: parent.width  - 60
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
                enabled: logic.radioButtonProtoOpenVpnTcpEnabled
                checked: logic.radioButtonProtoOpenVpnTcpChecked
                onCheckedChanged: {
                    UiLogic.radioButtonProtoOpenVpnTcpChecked = checked
                }
            }
            RadioButtonType {
                x: 10
                y: 10
                width: 171
                height: 19
                text: qsTr("UDP")
                checked: logic.radioButtonProtoOpenVpnUdpChecked
                onCheckedChanged: {
                    logic.radioButtonProtoOpenVpnUdpChecked = checked
                }
                enabled: logic.radioButtonProtoOpenVpnUdpEnabled
            }
        }

        //
        LabelType {
            id: lb_port
            anchors.top: rect_proto.bottom
            anchors.topMargin: 20

            x: 30
            width: root.width / 2 - 10
            height: 31
            text: qsTr("Port")
        }
        TextFieldType {
            id: tf_port
            anchors.top: rect_proto.bottom
            anchors.topMargin: 20

            anchors.left: parent.horizontalCenter
            anchors.right: parent.right
            anchors.rightMargin: 30

            height: 31
            text: logic.lineEditProtoOpenVpnPortText
            onEditingFinished: {
                logic.lineEditProtoOpenVpnPortText = text
            }
            enabled: logic.lineEditProtoOpenVpnPortEnabled
        }

        //
        CheckBoxType {
            id: check_auto_enc
            anchors.top: lb_port.bottom
            anchors.topMargin: 20
            x: 30
            width: parent.width
            height: 21
            text: qsTr("Auto-negotiate encryption")
            checked: logic.checkBoxProtoOpenVpnAutoEncryptionChecked
            onCheckedChanged: {
                logic.checkBoxProtoOpenVpnAutoEncryptionChecked = checked
            }
            onClicked: {
                logic.checkBoxProtoOpenVpnAutoEncryptionClicked()
            }
        }


        //
        LabelType {
            id: lb_cipher
            x: 30
            anchors.top: check_auto_enc.bottom
            anchors.topMargin: 20
            width: parent.width
            height: 21
            text: qsTr("Cipher")
        }

        ComboBoxType {
            id: cb_cipher
            x: 30
            anchors.top: lb_cipher.bottom
            width: parent.width - 60

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
                    if (logic.comboBoxProtoOpenVpnCipherText === model[i]) {
                        return i
                    }
                }
                return -1
            }
            onCurrentTextChanged: {
                logic.comboBoxProtoOpenVpnCipherText = currentText
            }
            enabled: logic.comboBoxProtoOpenVpnCipherEnabled
        }

        //
        LabelType {
            id: lb_hash
            anchors.top: cb_cipher.bottom
            anchors.topMargin: 20
            width: parent.width

            height: 21
            text: qsTr("Hash")
        }
        ComboBoxType {
            id: cb_hash
            x: 30
            height: 31
            anchors.top: lb_hash.bottom
            width: parent.width - 60
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
                    if (logic.comboBoxProtoOpenVpnHashText === model[i]) {
                        return i
                    }
                }
                return -1
            }
            onCurrentTextChanged: {
                logic.comboBoxProtoOpenVpnHashText = currentText
            }
            enabled: logic.comboBoxProtoOpenVpnHashEnabled
        }

        CheckBoxType {
            id: check_tls
            x: 30
            anchors.top: cb_hash.bottom
            anchors.topMargin: 20
            width: parent.width
            height: 21
            text: qsTr("Enable TLS auth")
            checked: logic.checkBoxProtoOpenVpnTlsAuthChecked
            onCheckedChanged: {
                logic.checkBoxProtoOpenVpnTlsAuthChecked = checked
            }

        }

        CheckBoxType {
            id: check_block_dns
            x: 30
            anchors.top: check_tls.bottom
            anchors.topMargin: 20
            width: parent.width
            height: 21
            text: qsTr("Block DNS requests outside of VPN")
            checked: logic.checkBoxProtoOpenVpnBlockDnsChecked
            onCheckedChanged: {
                logic.checkBoxProtoOpenVpnBlockDnsChecked = checked
            }
        }




//        LabelType {
//            id: label_proto_openvpn_info
//            x: 30
//            y: 550
//            width: 321
//            height: 41
//            visible: logic.labelProtoOpenVpnInfoVisible
//            text: logic.labelProtoOpenVpnInfoText
//        }


        ProgressBar {
            id: progress_save
            anchors.horizontalCenter: parent.horizontalCenter
            y: 500
            width: 321
            height: 40
            from: 0
            to: logic.progressBarProtoOpenVpnResetMaximium
            value: logic.progressBarProtoOpenVpnResetValue
            visible: logic.progressBarProtoOpenVpnResetVisible
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
        BlueButtonType {
            id: pb_save
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            x: 30
            width: parent.width - 60
            height: 40
            text: qsTr("Save and restart VPN")
            visible: logic.pushButtonOpenvpnSaveVisible
            onClicked: {
                logic.onPushButtonProtoOpenVpnSaveClicked()
            }
        }
    }
}
