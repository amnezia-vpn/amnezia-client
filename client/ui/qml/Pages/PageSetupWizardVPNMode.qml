import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
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
        width: root.width
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: root.bottom
        anchors.bottomMargin: 20
        anchors.left: root.left
        anchors.leftMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 15

        contentHeight: content.height
        clip: true

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
You can enable VPN mode "For selected sites" and add blocked sites you need to visit manually. If you will choose this option, you will need add every bloked site you want to visit to the access list. You may switch between modes later.\n\nPlease note, you should add addresses to the list after VPN connection established. You may add any domain, URL or IP address, it will be resolved to IP address.')
            }

            CheckBoxType {
                Layout.fillWidth: true
                text: qsTr('Turn on mode "VPN for selected sites"')
                checked: WizardLogic.checkBoxVpnModeChecked
                onCheckedChanged: {
                    WizardLogic.checkBoxVpnModeChecked = checked
                }
            }

            BlueButtonType {
                id: vpn_mode_finish
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.preferredHeight: 41
                text: qsTr("Start configuring")
                onClicked: {
                    WizardLogic.onPushButtonVpnModeFinishClicked()
                }
            }
        }
    }
}
