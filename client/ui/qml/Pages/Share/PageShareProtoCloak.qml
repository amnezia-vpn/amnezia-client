import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.Cloak
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share Cloak Settings")
    }


    TextAreaType {
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: pb_save.top
        anchors.bottomMargin: 20
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60

        textArea.readOnly: true

        textArea.text: ShareConnectionLogic.plainTextEditShareCloakText
    }

    ShareConnectionButtonType {
        id: pb_save
        anchors.bottom: root.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60
        text: ShareConnectionLogic.pushButtonShareCloakCopyText
        //enabled: ShareConnectionLogic.pushButtonShareCloakCopyEnabled
        onClicked: {
            ShareConnectionLogic.onPushButtonShareCloakCopyClicked()
        }
    }


}
