import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.WireGuard
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("WireGuard Settings")
    }

    Flickable {
        id: fl
        width: root.width
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.left: root.left
        anchors.leftMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 30

        contentHeight: content.height
        clip: true

        ColumnLayout {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            TextAreaType {
                id: ta_config

                Layout.topMargin: 5
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                Layout.preferredHeight: fl.height - 70
                flickableDirection: Flickable.AutoFlickIfNeeded

                textArea.readOnly: true
                textArea.text: logic.wireGuardLastConfigText
            }
        }
    }

}
