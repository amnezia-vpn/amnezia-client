import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ServerSettings
    logic: ServerSettingsLogic

    enabled: ServerSettingsLogic.pageEnabled

    BackButton {
        id: back
    }
    Caption {
        text: qsTr("Server settings")
        anchors.horizontalCenter: parent.horizontalCenter
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
        text: ServerSettingsLogic.labelCurrentVpnProtocolText
    }
    LabelType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 120
        width: 341
        height: 31
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        text: ServerSettingsLogic.labelServerText
    }
    LabelType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 530
        width: 301
        height: 41
        text: ServerSettingsLogic.labelWaitInfoText
        visible: ServerSettingsLogic.labelWaitInfoVisible
    }
    TextFieldType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 80
        width: 251
        height: 31
        text: ServerSettingsLogic.lineEditDescriptionText
        onEditingFinished: {
            ServerSettingsLogic.lineEditDescriptionText = text
            ServerSettingsLogic.onLineEditDescriptionEditingFinished()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 410
        width: 300
        height: 40
        text: ServerSettingsLogic.pushButtonClearText
        visible: ServerSettingsLogic.pushButtonClearVisible
        onClicked: {
            ServerSettingsLogic.onPushButtonClearServer()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 350
        width: 300
        height: 40
        text: ServerSettingsLogic.pushButtonClearClientCacheText
        visible: ServerSettingsLogic.pushButtonClearClientCacheVisible
        onClicked: {
            ServerSettingsLogic.onPushButtonClearClientCacheClicked()
        }
    }
    BlueButtonType {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 470
        width: 300
        height: 40
        text: qsTr("Forget this server")
        onClicked: {
            ServerSettingsLogic.onPushButtonForgetServer()
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
        visible: ServerSettingsLogic.pushButtonShareFullVisible
        onClicked: {
            ServerSettingsLogic.onPushButtonShareFullClicked()
        }
    }
}
