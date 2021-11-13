import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.Any
    logic: ShareConnectionLogic

    readonly property string generateConfigText: qsTr("Generate config")
    readonly property string generatingConfigText: qsTr("Generating config...")
    readonly property string showConfigText: qsTr("Show config")
    property bool genConfigProcess: false

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share for Amnezia")
    }

    Flickable {
        id: fl
        width: root.width
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: root.bottom
        anchors.bottomMargin: 20
        anchors.left: root.left
        anchors.leftMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 30

        contentHeight: content.height + 20
        clip: true

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Text {
                id: lb_desc
                Layout.fillWidth: true
                font.family: "Lato"
                font.styleName: "normal"
                font.pixelSize: 16
                color: "#181922"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                text: ShareConnectionLogic.shareFullAccess
                      ? qsTr("Anyone who logs in with this code will have the same permissions to use VPN and YOUR SERVER as you. \n
This code includes your server credentials!\n
Provide this code only to TRUSTED users.")
                      : qsTr("Anyone who logs in with this code will be able to connect to this VPN server. \n
This code does not include server credentials.")

            }

            ShareConnectionButtonType {
                id: pb_gen
                Layout.topMargin: 20
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                text: ShareConnectionLogic.shareFullAccess
                      ? showConfigText
                      : (genConfigProcess ? generatingConfigText : generateConfigText)
                onClicked: {
                    enabled = false
                    genConfigProcess = true
                    ShareConnectionLogic.onPushButtonShareAmneziaGenerateClicked()
                    enabled = true
                    genConfigProcess = false
                }
            }

            TextAreaType {
                id: tfShareCode

                Layout.topMargin: 20
                Layout.bottomMargin: 20
                Layout.preferredHeight: 200

                Layout.fillWidth: true

                textArea.readOnly: true
                textArea.wrapMode: TextEdit.WrapAnywhere
                textArea.verticalAlignment: Text.AlignTop
                textArea.text: ShareConnectionLogic.textEditShareAmneziaCodeText

                visible: tfShareCode.textArea.length > 0
            }


            ShareConnectionButtonCopyType {
                id: pb_copy
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                enabled: tfShareCode.textArea.length > 0
                visible: tfShareCode.textArea.length > 0
            }
            ShareConnectionButtonType {
                id: pb_save
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: qsTr("Save to file")
                enabled: tfShareCode.textArea.length > 0
                visible: tfShareCode.textArea.length > 0

                onClicked: {
                    UiLogic.saveTextFile(qsTr("Save AmneziaVPN config"), "*.vpn", tfShareCode.textArea.text)
                }
            }

            Image {
                id: label_share_code
                Layout.topMargin: 20
                Layout.fillWidth: true
                Layout.preferredHeight: width
                smooth: false
                source: ShareConnectionLogic.shareAmneziaQrCodeText
            }
        }
    }
}
