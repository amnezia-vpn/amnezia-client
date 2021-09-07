import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    enabled: ServerSettingsLogic.pageServerSettingsEnabled

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
        anchors.horizontalCenter: parent.horizontalCenter
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
        text: ServerSettingsLogic.labelServerSettingsCurrentVpnProtocolText
    }
    LabelType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 120
        width: 341
        height: 31
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        text: ServerSettingsLogic.labelServerSettingsServerText
    }
    LabelType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 530
        width: 301
        height: 41
        text: ServerSettingsLogic.labelServerSettingsWaitInfoText
        visible: ServerSettingsLogic.labelServerSettingsWaitInfoVisible
    }
    TextFieldType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 80
        width: 251
        height: 31
        text: ServerSettingsLogic.lineEditServerSettingsDescriptionText
        onEditingFinished: {
            ServerSettingsLogic.lineEditServerSettingsDescriptionText = text
            ServerSettingsLogic.onLineEditServerSettingsDescriptionEditingFinished()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 410
        width: 300
        height: 40
        text: ServerSettingsLogic.pushButtonServerSettingsClearText
        visible: ServerSettingsLogic.pushButtonServerSettingsClearVisible
        onClicked: {
            ServerSettingsLogic.onPushButtonServerSettingsClearServer()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 350
        width: 300
        height: 40
        text: ServerSettingsLogic.pushButtonServerSettingsClearClientCacheText
        visible: ServerSettingsLogic.pushButtonServerSettingsClearClientCacheVisible
        onClicked: {
            ServerSettingsLogic.onPushButtonServerSettingsClearClientCacheClicked()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 470
        width: 300
        height: 40
        text: qsTr("Forget this server")
        onClicked: {
            ServerSettingsLogic.onPushButtonServerSettingsForgetServer()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 210
        width: 300
        height: 40
        text: qsTr("VPN protocols")
        onClicked: {
            UiLogic.goToPage(PageEnum.ServerContainers)
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 260
        width: 300
        height: 40
        text: qsTr("Share Server (FULL ACCESS)")
        visible: ServerSettingsLogic.pushButtonServerSettingsShareFullVisible
        onClicked: {
            ServerSettingsLogic.onPushButtonServerSettingsShareFullClicked()
        }
    }
}
