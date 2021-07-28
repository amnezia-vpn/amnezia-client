import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"

Item {
    id: root
    width: GC.screenWidth
    height: GC.screenHeight
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
        text: qsTr("Server settings")
        x: 10
        y: 35
        width: 361
        height: 31
    }
    Image {
        anchors.horizontalCenter: root.horizontalCenter
        width: GC.trW(150)
        height: GC.trH(22)
        y: GC.trY(590)
        source: "qrc:/images/AmneziaVPN.png"
    }
    LabelType {
        x: 20
        y: 150
        width: 341
        height: 31
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        text: UiLogic.labelServerSettingsCurrentVpnProtocolText
    }
    LabelType {
        x: 20
        y: 120
        width: 341
        height: 31
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        text: UiLogic.labelServerSettingsServerText
    }
    LabelType {
        x: 40
        y: 530
        width: 301
        height: 41
        text: UiLogic.labelServerSettingsWaitInfoText
        visible: UiLogic.labelServerSettingsWaitInfoVisible
    }
    TextFieldType {
//        x: 70
        anchors.horizontalCenter: parent.horizontalCenter
        y: 80
        width: 251
        height: 31
        text: UiLogic.lineEditServerSettingsDescriptionText
        onEditingFinished: {
            UiLogic.lineEditServerSettingsDescriptionText = text
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 410
        width: 300
        height: 40
        text: qsTr("Clear server from Amnezia software")
        visible: UiLogic.pushButtonServerSettingsClearVisible
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 350
        width: 300
        height: 40
        text: qsTr("Clear client cached profile")
        visible: UiLogic.pushButtonServerSettingsClearClientCacheVisible
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 470
        width: 300
        height: 40
        text: qsTr("Forget this server")
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 210
        width: 300
        height: 40
        text: qsTr("VPN protocols")
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 260
        width: 300
        height: 40
        text: qsTr("Share Server (FULL ACCESS)")
        visible: UiLogic.pushButtonServerSettingsShareFullVisible
    }
}
