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
        text: qsTr("Setup your server to use VPN")
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
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignTop
            text: qsTr("I'm living in country with high censorship level. Many of foreign web sites and VPNs blocked by my government. I want to setup reliable VPN, which is invisible for government.")
            wrapMode: Text.Wrap
            x: 30
            y: 40
            width: 321
            height: 121
        }
        LabelType {
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignTop
            text: qsTr("I'm living in country with medium censorship level. Some web sites blocked by my government, but VPNs are not blocked at all. I want to setup flexible solution.")
            wrapMode: Text.Wrap
            x: 30
            y: 210
            width: 321
            height: 121
        }
        LabelType {
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignTop
            text: qsTr("I just want to improve my privacy in internet.")
            wrapMode: Text.Wrap
            x: 30
            y: 360
            width: 321
            height: 121
        }
        BlueButtonType {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 490
            width: 321
            height: 40
            text: qsTr("Next")
            onClicked: {
                if (radioButton_setup_wizard_high.checked) {
                    UiLogic.goToPage(PageEnum.WizardHigh);
                } else if (radioButton_setup_wizard_medium.checked) {
                    UiLogic.goToPage(PageEnum.WizardMedium);
                } else if (radioButton_setup_wizard_low.checked) {
                    UiLogic.goToPage(PageEnum.WizardLow);
                }
            }
        }
        RadioButtonType {
            id: radioButton_setup_wizard_high
            x: 10
            y: 10
            width: 331
            height: 25
            text: qsTr("High censorship level")
            checked: UiLogic.radioButtonSetupWizardHighChecked
            onCheckedChanged: {
                UiLogic.radioButtonSetupWizardHighChecked = checked
            }
        }
        RadioButtonType {
            id: radioButton_setup_wizard_medium
            x: 10
            y: 330
            width: 331
            height: 25
            text: qsTr("Low censorship level")
            checked: UiLogic.radioButtonSetupWizardLowChecked
            onCheckedChanged: {
                UiLogic.radioButtonSetupWizardLowChecked = checked
            }
        }
        RadioButtonType {
            id: radioButton_setup_wizard_low
            x: 10
            y: 180
            width: 331
            height: 25
            text: qsTr("Medium censorship level")
            checked: UiLogic.radioButtonSetupWizardMediumChecked
            onCheckedChanged: {
                UiLogic.radioButtonSetupWizardMediumChecked = checked
            }
        }
    }
}
