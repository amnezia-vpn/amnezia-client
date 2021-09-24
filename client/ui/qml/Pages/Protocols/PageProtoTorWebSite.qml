import QtQuick 2.12
import QtQuick.Controls 2.12
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.TorWebSite
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("TOR Web site settings")
    }

    Rectangle {
        id: frame_settings
        width: parent.width
        anchors.top: caption.bottom
        anchors.topMargin: 10

        border.width: 1
        border.color: "lightgray"
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 2
        Grid {
            id: grid
            anchors.fill: parent
            columns: 2
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
            topPadding: 5
            leftPadding: 30
            rightPadding: 30
            spacing: 5


            LabelType {
                width: 130
                text: qsTr("Web site onion address")
            }
            TextFieldType {
                id: tf_site_address
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                text: logic.labelTorWebSiteAddressText
                readOnly: true
            }

        }
    }
}
