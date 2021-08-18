import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    ImageButtonType {
        id: back_from_setup_wizard
        x: 10
        y: 10
        width: 26
        height: 20
        icon.source: "qrc:/images/arrow_left.png"
        onClicked: {
            UiLogic.closePage()
        }
    }
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 24
        color: "#100A44"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Setup Wizard")
        x: 10
        y: 35
        width: 361
        height: 31
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
            height: 341
            verticalAlignment: Text.AlignTop
            text: qsTr('AmneziaVPN will install VPN protocol which is difficult to detect by your internet provider and government firewall (but possible). In most cases, this is the most suitable protocol. This protocol is faster compared to the VPN protocols with "web traffic masking".\n\nThis protocol support export connection profile to mobile devices using QR code (you should launch 3rd party opensource VPN client - ShadowSocks VPN).')
        }
        LabelType {
            x: 30
            y: 400
            width: 321
            height: 71
            text: qsTr('OpenVPN over ShadowSocks profile will be installed')
            verticalAlignment: Text.AlignBottom
        }
        BlueButtonType {
            id: next_button
            x: 30
            y: 490
            width: 301
            height: 40
            text: qsTr("Next")
            onClicked: {
                UiLogic.goToPage(PageEnum.WizardVpnMode)
            }
        }
    }
}
