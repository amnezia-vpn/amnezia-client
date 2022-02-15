import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.Ikev2

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Share IKEv2 Settings")
    }

    TextAreaType {
        id: tfCert
        textArea.readOnly: true
        textArea.text: ShareConnectionLogic.textEditShareIkev2CertText

        visible: false
    }

    TextAreaType {
        id: tfMobileConfig
        textArea.readOnly: true
        textArea.text: ShareConnectionLogic.textEditShareIkev2MobileConfigText

        visible: false
    }

    TextAreaType {
        id: tfStrongSwanConfig
        textArea.readOnly: true
        textArea.text: ShareConnectionLogic.textEditShareIkev2StrongSwanConfigText

        visible: false
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
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

//            LabelType {
//                id: lb_desc
//                Layout.fillWidth: true
//                Layout.topMargin: 10

//                horizontalAlignment: Text.AlignHCenter

//                wrapMode: Text.Wrap
//                text: qsTr("Note: ShadowSocks protocol using same password for all connections")
//            }

            ShareConnectionButtonType {
                Layout.topMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                text: genConfigProcess ? generatingConfigText : generateConfigText
                onClicked: {
                    enabled = false
                    genConfigProcess = true
                    ShareConnectionLogic.onPushButtonShareIkev2GenerateClicked()
                    enabled = true
                    genConfigProcess = false
                }
            }

            ShareConnectionButtonType {
                Layout.topMargin: 30
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                width: parent.width - 60

                text: qsTr("Export p12 certificate")
                enabled: tfCert.textArea.length > 0
                visible: tfCert.textArea.length > 0

                onClicked: {
                    UiLogic.saveTextFile(qsTr("Export p12 certificate"), "amnezia_ikev2_cert_for_windows.p12", "*.p12", tfCert.textArea.text)
                }
            }

            ShareConnectionButtonType {
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                width: parent.width - 60

                text: qsTr("Export config for Apple")
                enabled: tfMobileConfig.textArea.length > 0
                visible: tfMobileConfig.textArea.length > 0

                onClicked: {
                    UiLogic.saveTextFile(qsTr("Export config for Apple"), "amnezia_for_apple.plist", "*.plist", tfMobileConfig.textArea.text)
                }
            }

            ShareConnectionButtonType {
                Layout.bottomMargin: 10
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                width: parent.width - 60

                text: qsTr("Export config for StrongSwan")
                enabled: tfStrongSwanConfig.textArea.length > 0
                visible: tfStrongSwanConfig.textArea.length > 0

                onClicked: {
                    UiLogic.saveTextFile(qsTr("Export config for StrongSwan"), "amnezia_for_StrongSwan.profile", "*.profile", tfStrongSwanConfig.textArea.text)
                }
            }
        }
    }
}
