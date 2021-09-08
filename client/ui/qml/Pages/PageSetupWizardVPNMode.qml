import QtQuick 2.12
import QtQuick.Controls 2.12
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
        CheckBoxType {
            x: 30
            y: 350
            width: 301
            height: 71
            text: qsTr('Turn on mode "VPN for selected sites"')
            checked: WizardLogic.checkBoxVpnModeChecked
            onCheckedChanged: {
                WizardLogic.checkBoxVpnModeChecked = checked
            }
        }
        LabelType {
            x: 30
            y: 10
            width: 321
            height: 341
            text: qsTr('Optional.\n\nWe recommend to enable VPN mode "For selected sites" and add blocked sites you need to visit manually. If you will choose this option, you will need add every bloked site you want to visit to the access list. You may switch between modes later.\n\nPlease note, you should add addresses to the list after VPN connection established. You may add any domain, URL or IP address, it will be resolved to IP address.')
        }
        BlueButtonType {
            id: vpn_mode_finish
            x: 30
            y: 490
            width: 301
            height: 40
            text: qsTr("Start configuring")
            onClicked: {
                WizardLogic.onPushButtonVpnModeFinishClicked()
            }
        }
    }
}
