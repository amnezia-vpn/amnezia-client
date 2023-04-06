import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.Cloak

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share Cloak Settings")
    }

    FlickableType {
        id: fl
        anchors.top: caption.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 15

            LabelType {
                id: lb_desc
                Layout.fillWidth: true
                Layout.topMargin: 10

                horizontalAlignment: Text.AlignHCenter

                wrapMode: Text.Wrap
                text: qsTr("Note: Cloak protocol using same password for all connections")
            }

            ShareConnectionButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                text: genConfigProcess ? generatingConfigText : generateConfigText
                onClicked: {
                    enabled = false
                    genConfigProcess = true
                    ShareConnectionLogic.onPushButtonShareCloakGenerateClicked()
                    enabled = true
                    genConfigProcess = false
                }
            }

            TextAreaType {
                id: tfShareCode

                Layout.topMargin: 20
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.preferredHeight: 200

                textArea.readOnly: true
                textArea.text: ShareConnectionLogic.textEditShareCloakText

                visible: tfShareCode.textArea.length > 0
            }

            ShareConnectionButtonCopyType {
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                copyText: tfShareCode.textArea.text
            }

            ShareConnectionButtonType {
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                text: Qt.platform.os === "android" ? qsTr("Share") : qsTr("Save to file")
                enabled: tfShareCode.textArea.length > 0
                visible: tfShareCode.textArea.length > 0

                onClicked: {
                    UiLogic.saveTextFile(qsTr("Save AmneziaVPN config"), "amnezia_config_cloak.json", "*.json", tfShareCode.textArea.text)
                }
            }

        }
    }

}
