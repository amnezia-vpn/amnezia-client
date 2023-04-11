import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageClientInfoBase {
    id: root
    protocol: ProtocolEnum.WireGuard

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

    FlickableType {
        id: fl
        anchors.top: caption.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: GC.defaultMargin

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
                text: qsTr("Public Key")
            }

            TextAreaType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.preferredHeight: 200
                Layout.fillWidth: true

                textArea.readOnly: true
                textArea.wrapMode: TextEdit.WrapAnywhere
                textArea.verticalAlignment: Text.AlignTop
                textArea.text: ClientInfoLogic.textAreaWireGuardKeyData
            }

            BlueButtonType {
                enabled: !ClientInfoLogic.busyIndicatorIsRunning
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                text: qsTr("Revoke Key")
                onClicked: {
                    ClientInfoLogic.onRevokeWireGuardKeyClicked()
                    UiLogic.closePage()
                }
            }
        }
    }
}
