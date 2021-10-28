import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.ShadowSocks
    logic: UiLogic.protocolLogic(protocol)

    enabled: logic.pageEnabled
    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("ShadowSocks Settings")
    }


    ColumnLayout {
        id: content
        enabled: logic.pageEnabled
        anchors.top: caption.bottom
        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: pb_save.top
        anchors.margins: 20
        anchors.topMargin: 10



        RowLayout {
            Layout.fillWidth: true

            LabelType {
                height: 31
                text: qsTr("Cipher")
                Layout.preferredWidth: 0.3 * root.width - 10
            }

            ComboBoxType {
                height: 31
                Layout.fillWidth: true

                model: [
                    qsTr("chacha20-ietf-poly1305"),
                    qsTr("xchacha20-ietf-poly1305"),
                    qsTr("aes-256-gcm"),
                    qsTr("aes-192-gcm"),
                    qsTr("aes-128-gcm")
                ]
                currentIndex: {
                    for (let i = 0; i < model.length; ++i) {
                        if (logic.comboBoxCipherText === model[i]) {
                            return i
                        }
                    }
                    return -1
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LabelType {
                Layout.preferredWidth: 0.3 * root.width - 10
                height: 31
                text: qsTr("Port")
            }

            TextFieldType {
                id: lineEdit_proto_shadowsocks_port
                Layout.fillWidth: true
                height: 31
                text: logic.lineEditPortText
                onEditingFinished: {
                    logic.lineEditPortText = text
                }
                enabled: logic.lineEditPortEnabled
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }


    LabelType {
        id: label_proto_shadowsocks_info
        x: 30
        anchors.bottom: pb_save.top
        anchors.bottomMargin: 10
        width: parent.width - 40
        height: 41
        visible: logic.labelInfoVisible
        text: logic.labelInfoText
    }

    ProgressBar {
        id: progressBar_reset
        anchors.fill: pb_save
        from: 0
        to: logic.progressBaResetMaximium
        value: logic.progressBaResetValue
        visible: logic.progressBaResetVisible
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
                width: progressBar_reset.visualPosition * parent.width
                height: parent.height
                radius: 4
                color: Qt.rgba(255, 255, 255, 0.15);
            }
        }
    }

    BlueButtonType {
        id: pb_save
        enabled: logic.pageEnabled
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: root.bottom
        anchors.bottomMargin: 20
        width: root.width - 60
        height: 40
        text: qsTr("Save and restart VPN")
        visible: logic.pushButtonSaveVisible
        onClicked: {
            logic.onPushButtonSaveClicked()
        }
    }
}
