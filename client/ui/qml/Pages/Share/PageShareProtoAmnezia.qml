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

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share for Amnezia")
    }

    Text {
        id: lb_desc
        anchors.top: caption.bottom
        anchors.topMargin: 20
        width: parent.width - 60
        anchors.horizontalCenter: root.horizontalCenter

        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 16
        color: "#181922"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        text: qsTr("Anyone who logs in with this code will be able to connect to this VPN server. \nThis code does not include server credentials.")
    }

    TextAreaType {
        anchors.top: lb_desc.bottom
        anchors.topMargin: 20
        anchors.bottom: pb_gen.top
        anchors.bottomMargin: 20

        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60

        textArea.readOnly: true
        textArea.wrapMode: TextEdit.WrapAnywhere
        textArea.verticalAlignment: Text.AlignTop
        textArea.text: ShareConnectionLogic.textEditShareAmneziaCodeText
    }


    ShareConnectionButtonType {
        id: pb_gen
        anchors.bottom: pb_copy.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60
        text: ShareConnectionLogic.pushButtonShareAmneziaGenerateText
        enabled: ShareConnectionLogic.pushButtonShareAmneziaGenerateEnabled
        onClicked: {
            ShareConnectionLogic.onPushButtonShareAmneziaGenerateClicked()
        }
    }
    ShareConnectionButtonType {
        id: pb_copy
        anchors.bottom: pb_save.top
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60
        text: ShareConnectionLogic.pushButtonShareAmneziaCopyText
        onClicked: {
            ShareConnectionLogic.onPushButtonShareAmneziaCopyClicked()
        }
        enabled: ShareConnectionLogic.pushButtonShareAmneziaCopyEnabled
    }
    ShareConnectionButtonType {
        id: pb_save
        anchors.bottom: root.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: parent.width - 60
        text: qsTr("Save file")
        onClicked: {
            ShareConnectionLogic.onPushButtonShareAmneziaSaveClicked()
        }
    }
}
