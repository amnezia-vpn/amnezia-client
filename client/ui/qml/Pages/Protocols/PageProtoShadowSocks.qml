import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageBase {
    id: root
    page: PageEnum.ShadowSocksSettings
    logic: ShadowSocksLogic

    enabled: ShadowSocksLogic.pageProtoShadowSocksEnabled
    BackButton {
        id: back
    }
    Item {
        x: 0
        y: 40
        width: 380
        height: 600
        enabled: ShadowSocksLogic.widgetProtoShadowSocksEnabled
        ComboBoxType {
            x: 190
            y: 60
            width: 151
            height: 31
            model: [
                qsTr("chacha20-poly1305"),
                qsTr("aes-256-gcm"),
                qsTr("aes-128-gcm")
            ]
            currentIndex: {
                for (let i = 0; i < model.length; ++i) {
                    if (ShadowSocksLogic.comboBoxProtoShadowSocksCipherText === model[i]) {
                        return i
                    }
                }
                return -1
            }
            onCurrentTextChanged: {
                ShadowSocksLogic.comboBoxProtoShadowSocksCipherText = currentText
            }
        }
        LabelType {
            x: 30
            y: 60
            width: 151
            height: 31
            text: qsTr("Cipher")
        }
        LabelType {
            x: 30
            y: 110
            width: 151
            height: 31
            text: qsTr("Port")
        }
        Text {
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 24
            color: "#100A44"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: qsTr("ShadowSocks Settings")
            x: 30
            y: 0
            width: 340
            height: 30
        }
        LabelType {
            id: label_proto_shadowsocks_info
            x: 30
            y: 550
            width: 321
            height: 41
            visible: ShadowSocksLogic.labelProtoShadowSocksInfoVisible
            text: ShadowSocksLogic.labelProtoShadowSocksInfoText
        }
        TextFieldType {
            id: lineEdit_proto_shadowsocks_port
            x: 190
            y: 110
            width: 151
            height: 31
            text: ShadowSocksLogic.lineEditProtoShadowSocksPortText
            onEditingFinished: {
                ShadowSocksLogic.lineEditProtoShadowSocksPortText = text
            }
            enabled: ShadowSocksLogic.lineEditProtoShadowSocksPortEnabled
        }
        ProgressBar {
            id: progressBar_proto_shadowsocks_reset
            anchors.horizontalCenter: parent.horizontalCenter
            y: 500
            width: 321
            height: 40
            from: 0
            to: ShadowSocksLogic.progressBarProtoShadowSocksResetMaximium
            value: ShadowSocksLogic.progressBarProtoShadowSocksResetValue
            visible: ShadowSocksLogic.progressBarProtoShadowSocksResetVisible
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
                    width: progressBar_proto_shadowsocks_reset.visualPosition * parent.width
                    height: parent.height
                    radius: 4
                    color: Qt.rgba(255, 255, 255, 0.15);
                }
            }
        }
        BlueButtonType {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 500
            width: 321
            height: 40
            text: qsTr("Save and restart VPN")
            visible: ShadowSocksLogic.pushButtonShadowSocksSaveVisible
            onClicked: {
                ShadowSocksLogic.onPushButtonProtoShadowSocksSaveClicked()
            }
        }
    }
}
