import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.Wizard
    logic: WizardLogic

    BackButton {
        id: back_from_setup_wizard
    }
    Caption {
        id: caption
        text: qsTr("Setup your server to use VPN")
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

            RadioButtonType {
                id: radioButton_setup_wizard_high
                Layout.fillWidth: true
                text: qsTr("High censorship level")
                checked: WizardLogic.radioButtonHighChecked
                onCheckedChanged: {
                    WizardLogic.radioButtonHighChecked = checked
                }
            }
            LabelType {
                Layout.fillWidth: true
                Layout.leftMargin: 25
                verticalAlignment: Text.AlignTop
                text: qsTr("I'm living in a country with a high censorship level. Many of the foreign websites and VPNs are blocked by my government. I want to setup a reliable VPN, which can not be detected by my internet provider and my government.
OpenVPN and ShadowSocks over Cloak (VPN obfuscation) profiles will be installed.\n")
            }


            RadioButtonType {
                id: radioButton_setup_wizard_medium
                Layout.fillWidth: true
                text: qsTr("Medium censorship level")
                checked: WizardLogic.radioButtonMediumChecked
                onCheckedChanged: {
                    WizardLogic.radioButtonMediumChecked = checked
                }
            }
            LabelType {
                Layout.fillWidth: true
                Layout.leftMargin: 25
                verticalAlignment: Text.AlignTop
                text: qsTr("I'm living in a country with a medium censorship level. Some websites are blocked by my government, but VPNs are not blocked at all. I want to setup a flexible solution.
OpenVPN over ShadowSocks profile will be installed.\n")
            }


            RadioButtonType {
                id: radioButton_setup_wizard_low
                Layout.fillWidth: true
                text: qsTr("Low censorship level")
                checked: WizardLogic.radioButtonLowChecked
                onCheckedChanged: {
                    WizardLogic.radioButtonLowChecked = checked
                }
            }
            LabelType {
                Layout.fillWidth: true
                Layout.leftMargin: 25
                verticalAlignment: Text.AlignTop
                text: qsTr("I want to improve my privacy on the internet.
OpenVPN profile will be installed.\n")
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
            if (radioButton_setup_wizard_high.checked) {
                UiLogic.goToPage(PageEnum.WizardHigh, false);
            } else if (radioButton_setup_wizard_medium.checked) {
                UiLogic.goToPage(PageEnum.WizardMedium, false);
            } else if (radioButton_setup_wizard_low.checked) {
                UiLogic.goToPage(PageEnum.WizardLow, false);
            }
        }
    }
}
