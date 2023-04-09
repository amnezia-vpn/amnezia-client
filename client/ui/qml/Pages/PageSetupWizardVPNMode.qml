import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.WizardVpnMode
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
        anchors.bottom: vpn_mode_finish.top
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
                text: qsTr('Optional.\n
You can enable VPN mode "For selected sites" and add blocked sites you need to visit manually. If you will choose this option, you will need add every blocked site you want to visit to the access list. You may switch between modes later.\n\nPlease note, you should add addresses to the list after VPN connection established. You may add any domain, URL or IP address, it will be resolved to IP address.')
            }

            CheckBoxType {
                Layout.fillWidth: true
                text: qsTr('Turn on mode "VPN for selected sites"')
                checked: WizardLogic.checkBoxVpnModeChecked
                onCheckedChanged: {
                    WizardLogic.checkBoxVpnModeChecked = checked
                }
            }
        }
    }

    BlueButtonType {
        id: vpn_mode_finish
        anchors.bottom: parent.bottom
        anchors.bottomMargin: GC.defaultMargin
        x: GC.defaultMargin
        width: parent.width - 2 * GC.defaultMargin
        text: qsTr("Start configuring")
        onClicked: {
            WizardLogic.onPushButtonVpnModeFinishClicked()
        }
    }
}
