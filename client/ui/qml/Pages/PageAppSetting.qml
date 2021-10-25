import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.AppSettings
    logic: AppSettingsLogic

    BackButton {
        id: back
    }
    Caption {
        text: qsTr("Application Settings")
    }
    CheckBoxType {
        x: 30
        y: 140
        width: 211
        height: 31
        text: qsTr("Auto connect")
        checked: AppSettingsLogic.checkBoxAutoConnectChecked
        onCheckedChanged: {
            AppSettingsLogic.checkBoxAutoConnectChecked = checked
            AppSettingsLogic.onCheckBoxAutoconnectToggled(checked)
        }
    }
    CheckBoxType {
        x: 30
        y: 100
        width: 211
        height: 31
        text: qsTr("Auto start")
        checked: AppSettingsLogic.checkBoxAutostartChecked
        onCheckedChanged: {
            AppSettingsLogic.checkBoxAutostartChecked = checked
            AppSettingsLogic.onCheckBoxAutostartToggled(checked)
        }
    }
    CheckBoxType {
        x: 30
        y: 180
        width: 211
        height: 31
        text: qsTr("Start minimized")
        checked: AppSettingsLogic.checkBoxStartMinimizedChecked
        onCheckedChanged: {
            AppSettingsLogic.checkBoxStartMinimizedChecked = checked
            AppSettingsLogic.onCheckBoxStartMinimizedToggled(checked)
        }
    }
    LabelType {
        x: 30
        y: 240
        width: 281
        height: 21
        text: AppSettingsLogic.labelVersionText
    }
    BlueButtonType {
        x: 30
        y: 280
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 40
        height: 41
        text: qsTr("Check for updates")
        onClicked: {
            Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
        }
    }
    BlueButtonType {
        x: 30
        y: 340
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 40
        height: 41
        text: qsTr("Open logs folder")
        onClicked: {
            AppSettingsLogic.onPushButtonOpenLogsClicked()
        }
    }

    Logo {
        anchors.bottom: parent.bottom
    }
}
