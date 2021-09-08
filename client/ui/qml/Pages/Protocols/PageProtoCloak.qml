import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../../Controls"
import "../../Config"

Item {
    id: root
    enabled: CloakLogic.pageProtoCloakEnabled
    ImageButtonType {
        id: back
        x: 10
        y: 10
        width: 26
        height: 20
        icon.source: "qrc:/images/arrow_left.png"
        onClicked: {
            UiLogic.closePage()
        }
    }
    Item {
        x: 0
        y: 40
        width: 380
        height: 600
        enabled: CloakLogic.widgetProtoCloakEnabled
        ComboBoxType {
            x: 190
            y: 60
            width: 151
            height: 31
            model: [
                qsTr("chacha20-poly1305"),
                qsTr("aes-256-gcm"),
                qsTr("aes-192-gcm"),
                qsTr("aes-128-gcm")
            ]
            currentIndex: {
                for (let i = 0; i < model.length; ++i) {
                    if (CloakLogic.comboBoxProtoCloakCipherText === model[i]) {
                        return i
                    }
                }
                return -1
            }
            onCurrentTextChanged: {
                CloakLogic.comboBoxProtoCloakCipherText = currentText
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
            y: 160
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
            text: qsTr("Cloak Settings")
            x: 20
            y: 0
            width: 340
            height: 30
        }
        LabelType {
            x: 30
            y: 110
            width: 151
            height: 31
            text: qsTr("Fake Web Site")
        }

        LabelType {
            id: label_proto_cloak_info
            x: 30
            y: 550
            width: 321
            height: 41
            visible: CloakLogic.labelProtoCloakInfoVisible
            text: CloakLogic.labelProtoCloakInfoText
        }
        TextFieldType {
            id: lineEdit_proto_cloak_port
            x: 190
            y: 160
            width: 151
            height: 31
            text: CloakLogic.lineEditProtoCloakPortText
            onEditingFinished: {
                CloakLogic.lineEditProtoCloakPortText = text
            }
            enabled: CloakLogic.lineEditProtoCloakPortEnabled
        }
        TextFieldType {
            id: lineEdit_proto_cloak_site
            x: 190
            y: 110
            width: 151
            height: 31
            text: CloakLogic.lineEditProtoCloakSiteText
            onEditingFinished: {
                CloakLogic.lineEditProtoCloakSiteText = text
            }
        }
        ProgressBar {
            id: progressBar_proto_cloak_reset
            anchors.horizontalCenter: parent.horizontalCenter
            y: 500
            width: 321
            height: 40
            from: 0
            to: CloakLogic.progressBarProtoCloakResetMaximium
            value: CloakLogic.progressBarProtoCloakResetValue
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
            visible: CloakLogic.progressBarProtoCloakResetVisible
        }
        BlueButtonType {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 500
            width: 321
            height: 40
            text: qsTr("Save and restart VPN")
            visible: CloakLogic.pushButtonCloakSaveVisible
            onClicked: {
                CloakLogic.onPushButtonProtoCloakSaveClicked()
            }
        }
    }
}
