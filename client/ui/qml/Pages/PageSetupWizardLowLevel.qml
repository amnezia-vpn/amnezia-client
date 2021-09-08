import QtQuick 2.12
import QtQuick.Controls 2.12
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
            height: 341
            verticalAlignment: Text.AlignTop
            text: qsTr('AmneziaVPN will install OpenVPN protocol with public/private key pairs generated on server and client sides. You can also configure connection on your mobile device by copying exported ".ovpn" file to your device and setting up official OpenVPN client. We recommend do not use messengers for sending connection profile - it contains VPN private keys.')
        }
        LabelType {
            x: 30
            y: 400
            width: 321
            height: 71
            text: qsTr('OpenVPN profile will be installed')
            verticalAlignment: Text.AlignBottom
        }
        BlueButtonType {
            id: next_button
            x: 30
            y: 490
            width: 301
            height: 40
            text: qsTr("Start configuring")
            onClicked: {
                WizardLogic.onPushButtonLowFinishClicked()
            }
        }
    }
}
