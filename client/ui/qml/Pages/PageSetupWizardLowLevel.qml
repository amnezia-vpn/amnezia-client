import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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

    FlickableType {
        id: fl
        anchors.top: caption.bottom
        anchors.bottom: next_button.top
        contentHeight: content.height

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 15

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
        }
    }

    BlueButtonType {
        id: next_button
        anchors.bottom: parent.bottom
        anchors.bottomMargin: GC.defaultMargin
        x: GC.defaultMargin
        width: parent.width - 2 * GC.defaultMargin
        text: qsTr("Start configuring")
        onClicked: {
            WizardLogic.onPushButtonLowFinishClicked()
        }
    }
}
