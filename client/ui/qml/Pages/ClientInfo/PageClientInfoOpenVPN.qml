import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageClientInfoBase {
    id: root
    protocol: ProtocolEnum.OpenVpn

    BackButton {
        id: back
        enabled: !ClientInfoLogic.busyIndicatorIsRunning
    }

    Caption {
        id: caption
        text: qsTr("Client Info")
    }

    BusyIndicator {
        z: 99
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: ClientInfoLogic.busyIndicatorIsRunning
        running: ClientInfoLogic.busyIndicatorIsRunning
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
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            LabelType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.fillWidth: true
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                text: ClientInfoLogic.labelCurrentVpnProtocolText
            }

            LabelType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                height: 21
                text: qsTr("Client name")
            }

            TextFieldType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.fillWidth: true
                Layout.preferredHeight: 31
                text: ClientInfoLogic.lineEditNameAliasText
                onEditingFinished: {
                    if (text !== ClientInfoLogic.lineEditNameAliasText) {
                        ClientInfoLogic.lineEditNameAliasText = text
                        ClientInfoLogic.onLineEditNameAliasEditingFinished()
                    }
                }
            }

            LabelType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.topMargin: 20
                height: 21
                text: qsTr("Certificate id")
            }

            LabelType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: ClientInfoLogic.labelOpenVpnCertId
            }

            LabelType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.topMargin: 20
                height: 21
                text: qsTr("Certificate")
            }

            TextAreaType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.preferredHeight: 200
                Layout.fillWidth: true

                textArea.readOnly: true
                textArea.wrapMode: TextEdit.WrapAnywhere
                textArea.verticalAlignment: Text.AlignTop
                textArea.text: ClientInfoLogic.textAreaOpenVpnCertData
            }

            BlueButtonType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Revoke Certificate")
                onClicked: {
                    ClientInfoLogic.onRevokeOpenVpnCertificateClicked()
                }
            }
        }
    }
}
