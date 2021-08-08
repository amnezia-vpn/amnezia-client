import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
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
    Image {
        x: 10
        y: 160
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 220
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 620
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 560
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 280
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 100
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 340
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 400
        width: 360
        height: 1
        source: "qrc:/images/line.png"
    }
    Image {
        x: 10
        y: 40
        width: 360
        height: 1
        source: "qrc:/images/line.png"
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
        y: 575
        width: 330
        height: 30
        icon.source: "qrc:/images/settings.png"
        text: qsTr("Exit")
        onClicked: {
            Qt.quit()
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
            UiLogic.onPushButtonGeneralSettingsServerSettingsClicked()
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
        enabled: UiLogic.pushButtonGeneralSettingsShareConnectionEnable
        onClicked: {
            UiLogic.onPushButtonGeneralSettingsShareConnectionClicked()
        }
    }
}
