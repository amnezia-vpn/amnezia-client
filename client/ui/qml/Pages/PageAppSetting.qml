import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    ImageButtonType {
        id: back
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
        text: qsTr("Application Settings")
        x: 10
        y: 35
        width: 361
        height: 31
    }
    CheckBoxType {
        x: 30
        y: 140
        width: 211
        height: 31
        text: qsTr("Auto connect")
        checked: UiLogic.checkBoxAppSettingsAutoconnectChecked
        onCheckedChanged: {
            UiLogic.checkBoxAppSettingsAutoconnectChecked = checked
            UiLogic.onCheckBoxAppSettingsAutoconnectToggled(checked)
        }
    }
    CheckBoxType {
        x: 30
        y: 100
        width: 211
        height: 31
        text: qsTr("Auto start")
        checked: UiLogic.checkBoxAppSettingsAutostartChecked
        onCheckedChanged: {
            UiLogic.checkBoxAppSettingsAutostartChecked = checked
            UiLogic.onCheckBoxAppSettingsAutostartToggled(checked)
        }
    }
    CheckBoxType {
        x: 30
        y: 180
        width: 211
        height: 31
        text: qsTr("Start minimized")
        checked: UiLogic.checkBoxAppSettingsStartMinimizedChecked
        onCheckedChanged: {
            UiLogic.checkBoxAppSettingsStartMinimizedChecked = checked
            UiLogic.onCheckBoxAppSettingsStartMinimizedToggled(checked)
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
        text: UiLogic.labelAppSettingsVersionText
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
            UiLogic.onPushButtonAppSettingsOpenLogsChecked()
        }
    }
}
