import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.WizardMedium
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
                text: qsTr('AmneziaVPN will install a VPN protocol which is difficult to detect by your internet provider and government firewall (but possible). In most cases, this is the most suitable protocol. This protocol is faster compared to the VPN protocols with "VPN masking".\n\nThis protocol supports exporting connection profiles to mobile devices by using QR codes (you should launch the 3rd party open source VPN client - ShadowSocks VPN).')
            }
            LabelType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                text: qsTr('ShadowSocks profile will be installed')
            }
        }
    }

    BlueButtonType {
        id: next_button
        anchors.bottom: parent.bottom
        anchors.bottomMargin: GC.defaultMargin
        x: GC.defaultMargin
        width: parent.width - 2 * GC.defaultMargin
        text: qsTr("Next")
        onClicked: {
            UiLogic.goToPage(PageEnum.WizardVpnMode, false)
        }
    }
}
