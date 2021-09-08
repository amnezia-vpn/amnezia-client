import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    BackButton {
        id: back_from_setup_wizard
    }
    Caption {
        text: qsTr("Setup Wizard")
    }
    Item {
        x: 10
        y: 70
        width: 361
        height: 561
        LabelType {
            x: 30
            y: 10
            width: 321
            height: 321
            text: qsTr("AmneziaVPN will install VPN protocol which is not visible for your internet provider and government firewall. Your VPN connection will be detected by your provider as regular web traffic to particular web site.\n\nYou SHOULD set this web site address to some foreign web site which is updatesnot blocked by your internet provider. Other words you need to type below some foreign web site address which is accessible without VPN.\n\nPlease note, this protocol still does not support export connection profile to mobile devices. Keep for updates.")
        }
        LabelType {
            x: 30
            y: 400
            width: 321
            height: 71
            text: qsTr("OpenVPN over Cloak (VPN obfuscation) profile will be installed")
        }
        LabelType {
            x: 30
            y: 330
            width: 291
            height: 21
            text: qsTr("Type web site address for mask")
        }
        TextFieldType {
            id: website_masking
            x: 30
            y: 360
            width: 301
            height: 41
            text: WizardLogic.lineEditHighWebsiteMaskingText
            onEditingFinished: {
                let _text = website_masking.text
                _text.replace("http://", "");
                _text.replace("https://", "");
                if (!_text) {
                    return
                }
                _text = _text.split("/").first();
                WizardLogic.lineEditHighWebsiteMaskingText = _text
            }
            onAccepted: {
                next_button.clicked()
            }
        }
        BlueButtonType {
            id: next_button
            x: 30
            y: 490
            width: 301
            height: 40
            text: qsTr("Next")
            onClicked: {
                let domain = website_masking.text;
                if (!domain || !domain.includes(".")) {
                    return
                }
                UiLogic.goToPage(PageEnum.WizardVpnMode)
            }
        }
    }
}
