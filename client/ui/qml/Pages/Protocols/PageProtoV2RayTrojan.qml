import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.V2RayTrojan
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
        enabled: !logic.pushButtonCancelVisible
    }

    Caption {
        id: caption
        text: qsTr("V2Ray Trojan Settings")
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
                Layout.preferredWidth: 0.3 * root.width - 10
                height: 31
                text: qsTr("Port")
            }

            TextFieldType {
                Layout.fillWidth: true
                height: 31
                text: logic.lineEditServerPortText
                onEditingFinished: {
                    logic.lineEditServerPortText = text
                }
                enabled: logic.lineEditServerPortEnabled
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LabelType {
                Layout.preferredWidth: 0.3 * root.width - 10
                height: 31
                text: qsTr("Local port")
            }

            TextFieldType {
                Layout.fillWidth: true
                height: 31
                text: logic.lineEditLocalPortText
                onEditingFinished: {
                    logic.lineEditLocalPortText = text
                }
                enabled: logic.lineEditLocalPortEnabled
            }
        }

        Item {
            Layout.fillHeight: true
        }

        LabelType {
            horizontalAlignment: Text.AlignHCenter
            Layout.maximumWidth: parent.width
            Layout.fillWidth: true
            visible: logic.labelServerBusyVisible
            text: logic.labelServerBusyText
        }

        LabelType {
            horizontalAlignment: Text.AlignHCenter
            Layout.maximumWidth: parent.width
            Layout.fillWidth: true
            visible: logic.labelInfoVisible
            text: logic.labelInfoText
        }
    }

    ProgressBar {
        id: progressBar_reset
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
                width: progressBar_reset.visualPosition * parent.width
                height: parent.height
                radius: 4
                color: Qt.rgba(255, 255, 255, 0.15);
            }
        }
        LabelType {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: logic.progressBarText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#D4D4D4"
            visible: logic.progressBarTextVisible
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

    BlueButtonType {
        anchors.fill: pb_save
        text: qsTr("Cancel")
        visible: logic.pushButtonCancelVisible
        enabled: logic.pushButtonCancelVisible
        onClicked: {
            logic.onPushButtonCancelClicked()
        }
    }
}
