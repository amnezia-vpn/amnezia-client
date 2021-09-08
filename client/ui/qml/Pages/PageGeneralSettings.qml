import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    BackButton {
        id: back
    }
    Rectangle {
        y: 40
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    Rectangle {
        y: 100
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    Rectangle {
        y: 160
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    Rectangle {
        y: 220
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    Rectangle {
        y: 280
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    Rectangle {
        y: 340
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    Rectangle {
        y: 400
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }

    SettingButtonType {
        x: 30
        y: 355
        width: 330
        height: 30
        icon.source: "qrc:/images/plus.png"
        text: qsTr("Add server")
        onClicked: {
            UiLogic.goToPage(PageEnum.Start)
        }
    }
    SettingButtonType {
        x: 30
        y: 55
        width: 330
        height: 30
        icon.source: "qrc:/images/settings.png"
        text: qsTr("App settings")
        onClicked: {
            UiLogic.goToPage(PageEnum.AppSettings)
        }
    }
    SettingButtonType {
        x: 30
        y: 115
        width: 330
        height: 30
        icon.source: "qrc:/images/settings.png"
        text: qsTr("Network settings")
        onClicked: {
            UiLogic.goToPage(PageEnum.NetworkSettings)
        }
    }
    SettingButtonType {
        x: 30
        y: 175
        width: 330
        height: 30
        icon.source: "qrc:/images/server_settings.png"
        text: qsTr("Server management")
        onClicked: {
            GeneralSettingsLogic.onPushButtonGeneralSettingsServerSettingsClicked()
        }
    }
    SettingButtonType {
        x: 30
        y: 295
        width: 330
        height: 30
        icon.source: "qrc:/images/server_settings.png"
        text: qsTr("Servers")
        onClicked: {
            UiLogic.goToPage(PageEnum.ServersList)
        }
    }
    SettingButtonType {
        x: 30
        y: 235
        width: 330
        height: 30
        icon.source: "qrc:/images/share.png"
        text: qsTr("Share connection")
        enabled: GeneralSettingsLogic.pushButtonGeneralSettingsShareConnectionEnable
        onClicked: {
            GeneralSettingsLogic.onPushButtonGeneralSettingsShareConnectionClicked()
        }
    }
    SettingButtonType {
        x: 30
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        width: 330
        height: 30
        icon.source: "qrc:/images/settings.png"
        text: qsTr("Exit")
        onClicked: {
            Qt.quit()
        }
    }
}
