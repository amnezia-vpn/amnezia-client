import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
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
                text: qsTr('AmneziaVPN will install a VPN protocol which is difficult to detect by your internet provider and government firewall (but possible). In most cases, this is the most suitable protocol. This protocol is faster compared to the VPN protocols with "VPN masking".\n\nThis protocol supports exporting connection profiles to mobile devices by using QR codes (you should launch the 3rd party open source VPN client - ShadowSocks VPN).')
            }
            LabelType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                text: qsTr('OpenVPN over ShadowSocks profile will be installed')
            }
            BlueButtonType {
                id: next_button
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.preferredHeight: 41
                text: qsTr("Next")
                onClicked: {
                    UiLogic.goToPage(PageEnum.WizardVpnMode, false)
                }
            }
        }
    }
}
