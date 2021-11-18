import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.Cloak
    logic: UiLogic.protocolLogic(protocol)

    enabled: logic.pageEnabled
    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Cloak Settings")
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
                Layout.fillWidth: true
                height: 31
                model: [
                    qsTr("chacha20-poly1305"),
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
                onCurrentTextChanged: {
                    logic.comboBoxCipherText = currentText
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LabelType {
                Layout.preferredWidth: 0.3 * root.width - 10
                height: 31
                text: qsTr("Fake Web Site")
            }

            TextFieldType {
                id: lineEdit_proto_cloak_site
                Layout.fillWidth: true
                height: 31
                text: logic.lineEditSiteText
                onEditingFinished: {
                    logic.lineEditSiteText = text
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
                id: lineEdit_proto_cloak_port
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
        id: label_proto_cloak_info
        x: 30
        anchors.bottom: pb_save.top
        anchors.bottomMargin: 10
        width: parent.width - 40
        visible: logic.labelInfoVisible
        text: logic.labelInfoText
    }


    ProgressBar {
        id: progressBar_proto_cloak_reset
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.fill: pb_save
        from: 0
        to: logic.progressBarResetMaximium
        value: logic.progressBarResetValue
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
                width: progressBar_proto_cloak_reset.visualPosition * parent.width
                height: parent.height
                radius: 4
                color: Qt.rgba(255, 255, 255, 0.15);
            }
        }
        visible: logic.progressBarResetVisible
    }
    BlueButtonType {
        id: pb_save
        anchors.horizontalCenter: parent.horizontalCenter
        enabled: logic.pageEnabled
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
