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
    logic: ShareConnectionLogic

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share OpenVPN Settings")
    }

    TextAreaType {
        id: tfShareCode
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: pb_gen.top
        anchors.bottomMargin: 20

        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60

        textArea.readOnly: true

        textArea.verticalAlignment: Text.AlignTop
        textArea.text: ShareConnectionLogic.textEditShareOpenVpnCodeText
    }


    ShareConnectionButtonType {
        id: pb_gen
        anchors.bottom: pb_copy.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60

        text: ShareConnectionLogic.pushButtonShareOpenVpnGenerateText
        onClicked: {
            enabled = false
            ShareConnectionLogic.onPushButtonShareOpenVpnGenerateClicked()
            enabled = true
        }
    }
    ShareConnectionButtonCopyType {
        id: pb_copy
        anchors.bottom: pb_save.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60

        enabled: tfShareCode.textArea.length > 0
    }
    ShareConnectionButtonType {
        id: pb_save
        anchors.bottom: root.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60

        text: qsTr("Save to file")
        enabled: tfShareCode.textArea.length > 0

        onClicked: {
            UiLogic.saveTextFile(qsTr("Save OpenVPN config"), "*.ovpn", tfShareCode.textArea.text)
        }
    }
}
