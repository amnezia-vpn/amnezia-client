import QtQuick 2.12
import QtQuick.Controls 2.12
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.Sftp
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("SFTF settings")
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
            leftPadding: 10
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
                text: qsTr("User Name")
            }
            TextFieldType {
                id: tf_user_name
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                text: logic.labelTftpUserNameText
                readOnly: true
            }

            LabelType {
                width: 130
                text: qsTr("Password")
            }
            TextFieldType {
                id: tf_password
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                text: logic.labelTftpPasswordText
                readOnly: true
            }
        }
    }

}
