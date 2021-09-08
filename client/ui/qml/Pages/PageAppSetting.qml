import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
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
    Image {
        anchors.horizontalCenter: root.horizontalCenter
        width: GC.trW(150)
        height: GC.trH(22)
        y: GC.trY(590)
        source: "qrc:/images/AmneziaVPN.png"
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
        width: 321
        height: 41
        text: qsTr("Check for updates")
        onClicked: {
            Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
        }
    }
    BlueButtonType {
        x: 30
        y: 340
        width: 321
        height: 41
        text: qsTr("Open logs folder")
        onClicked: {
            AppSettingsLogic.onPushButtonOpenLogsClicked()
        }
    }
}
