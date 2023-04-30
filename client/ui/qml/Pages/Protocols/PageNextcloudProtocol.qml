import QtQuick
import QtQuick.Controls
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.Nextcloud
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Nextcloud settings")
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
                text: qsTr("Port")
            }
            TextFieldType {
                id: tf_port_num
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                text: logic.labelTftpPortText
                readOnly: true
            }

            LabelType {
                width: 130
                text: qsTr("Admin user")
            }
            TextFieldType {
                id: tf_admin_user
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                text: logic.labelTftpPortText
                readOnly: true
            }

            LabelType {
                width: 130
                text: qsTr("Admin password")
            }
            TextFieldType {
                id: tf_admin_password
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                text: logic.labelTftpUserNameText
                readOnly: true
            }

        }
    }

}
