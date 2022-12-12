import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.OpenVpn

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share OpenVPN Settings")
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

        contentHeight: content.height
        clip: true

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right


            LabelType {
                id: lb_desc
                Layout.fillWidth: true
                Layout.topMargin: 10

                horizontalAlignment: Text.AlignHCenter

                wrapMode: Text.Wrap
                text: qsTr("New encryption keys pair will be generated.")
            }

            ShareConnectionButtonType {
                Layout.topMargin: 20
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                text: genConfigProcess ? generatingConfigText : generateConfigText
                onClicked: {
                    enabled = false
                    genConfigProcess = true
                    ShareConnectionLogic.onPushButtonShareOpenVpnGenerateClicked()
                    genConfigProcess = false
                    enabled = true
                }
            }

            TextAreaType {
                id: tfShareCode
                Layout.topMargin: 20
                Layout.preferredHeight: 200
                Layout.fillWidth: true

                textArea.readOnly: true

                textArea.verticalAlignment: Text.AlignTop
                textArea.text: ShareConnectionLogic.textEditShareOpenVpnCodeText

                visible: tfShareCode.textArea.length > 0
            }

            ShareConnectionButtonCopyType {
                Layout.preferredHeight: 40
                Layout.fillWidth: true

                copyText: tfShareCode.textArea.text
            }
            ShareConnectionButtonType {
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                width: parent.width - 60

                text: Qt.platform.os === "android" ? qsTr("Share") : qsTr("Save to file")
                enabled: tfShareCode.textArea.length > 0
                visible: tfShareCode.textArea.length > 0

                onClicked: {
                    UiLogic.saveTextFile(qsTr("Save OpenVPN config"), "amnezia_for_openvpn.ovpn", "*.ovpn", tfShareCode.textArea.text)
                }
            }
        }
    }
}
