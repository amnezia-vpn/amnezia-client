import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.ShadowSocks
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share ShadowSocks Settings")
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

        contentHeight: content.height + content2.height + 40
        clip: true

        GridLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            columns: 2

            //
            LabelType {
                height: 20
                text: qsTr("Server:")
            }
            TextFieldType {
                height: 20
                text: ShareConnectionLogic.labelShareShadowSocksServerText
                readOnly: true
            }

            //
            LabelType {
                height: 20
                text: qsTr("Port:")
            }
            TextFieldType {
                height: 20
                text: ShareConnectionLogic.labelShareShadowSocksPortText
                readOnly: true
            }

            //
            LabelType {
                height: 20
                text: qsTr("Password")
            }
            TextFieldType {
                height: 20
                text: ShareConnectionLogic.labelShareShadowSocksPasswordText
                readOnly: true
            }

            //
            LabelType {
                height: 20
                text: qsTr("Encryption:")
            }
            TextFieldType {
                height: 20
                text: ShareConnectionLogic.labelShareShadowSocksMethodText
                readOnly: true
            }
        }

        ColumnLayout {
            id: content2
            enabled: logic.pageEnabled
            anchors.top: content.bottom
            anchors.topMargin: 20
            anchors.left: parent.left
            anchors.right: parent.right

            LabelType {
                height: 20
                text: qsTr("Connection string")
            }
            TextFieldType {
                id: tfConnString
                height: 100
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                text: ShareConnectionLogic.lineEditShareShadowSocksStringText
                readOnly: true
            }
            ShareConnectionButtonType {
                height: 40
                Layout.fillWidth: true
                text: ShareConnectionLogic.pushButtonShareShadowSocksCopyText
                enabled: tfConnString.length > 0
                onClicked: {
                    ShareConnectionLogic.onPushButtonShareShadowSocksCopyClicked()
                }
            }

            Image {
                id: label_share_ss_qr_code
                Layout.topMargin: 20
                Layout.fillWidth: true
                Layout.preferredHeight: width
                smooth: false
                source: ShareConnectionLogic.labelShareShadowSocksQrCodeText
            }
        }
    }
}
