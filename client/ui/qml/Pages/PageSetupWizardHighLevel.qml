import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.WizardHigh
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
                text: qsTr("AmneziaVPN will install a VPN protocol which is not visible to your internet provider and government firewall. Your VPN connection will be seen by your internet provider as regular web traffic to a particular website.

You SHOULD set this website address to some foreign website which is not blocked by your internet provider. In other words, you need to type some foreign website address which is accessible to you without a VPN.")
            }

            LabelType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                verticalAlignment: Text.AlignTop
                text: qsTr("Type another web site address for masking or keep it by default. Your internet provider will think you working on this web site when you connected to VPN.")
            }

            TextFieldType {
                id: website_masking
                Layout.fillWidth: true
                text: WizardLogic.lineEditHighWebsiteMaskingText
                onEditingFinished: {
                    let _text = website_masking.text
                    _text = _text.replace("http://", "");
                    _text = _text.replace("https://", "");
                    if (!_text) {
                        return
                    }
                    _text = _text.split("/")[0];
                    WizardLogic.lineEditHighWebsiteMaskingText = _text
                }
                onAccepted: {
                    next_button.clicked()
                }
            }

            LabelType {
                Layout.fillWidth: true
                Layout.topMargin: 15
                verticalAlignment: Text.AlignTop
                text: qsTr("OpenVPN and ShadowSocks over Cloak (VPN obfuscation) profiles will be installed.

This protocol support exporting connection profiles to mobile devices by exporting ShadowSocks and Cloak configs (you should launch the 3rd party open source VPN client - ShadowSocks VPN and install Cloak plugin).")
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
            let domain = website_masking.text;
            if (!domain || !domain.includes(".")) {
                return
            }
            UiLogic.goToPage(PageEnum.WizardVpnMode, false)
        }
    }
}
