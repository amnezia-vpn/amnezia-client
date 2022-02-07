import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.WizardLow
    logic: WizardLogic

    BackButton {
        id: back_from_setup_wizard
    }
    Caption {
        id: caption
        text: qsTr("Setup Wizard")
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

            LabelType {
                Layout.fillWidth: true
                verticalAlignment: Text.AlignTop
                text: qsTr('AmneziaVPN will install the OpenVPN protocol with public/private key pairs generated on both server and client sides.

You can also configure the connection on your mobile device by copying the exported ".ovpn" file to your device, and setting up the official OpenVPN client.

We recommend not to use messaging applications for sending the connection profile - it contains VPN private keys.')
            }
            LabelType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                text: qsTr('OpenVPN profile will be installed')
                verticalAlignment: Text.AlignBottom
            }
            BlueButtonType {
                id: next_button
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.preferredHeight: 41
                text: qsTr("Start configuring")
                onClicked: {
                    WizardLogic.onPushButtonLowFinishClicked()
                }
            }
        }
    }
}
