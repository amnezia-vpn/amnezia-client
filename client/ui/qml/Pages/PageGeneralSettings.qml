import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.15
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.GeneralSettings
    logic: GeneralSettingsLogic

    BackButton {
        id: back
    }

    // ---------- App settings ------------
    Rectangle {
        id: l1
        visible: !GC.isMobile()
        anchors.top: back.bottom
        x: 20
        width: parent.width - 40
        height: GC.isMobile() ? 0: 1
        color: "#DDDDDD"
    }

    SettingButtonType {
        id: b1
        visible: !GC.isMobile()
        anchors.top: l1.bottom
        anchors.topMargin: GC.isMobile() ? 0: 15
        x: 30
        width: parent.width - 80
        height: GC.isMobile() ? 0: 30
        icon.source: "qrc:/images/svg/settings_black_24dp.svg"
        text: qsTr("App settings")
        onClicked: {
            UiLogic.goToPage(PageEnum.AppSettings)
        }
    }

    // ---------- Network settings ------------
    Rectangle {
        id: l2
        anchors.top: b1.bottom
        anchors.topMargin: 15
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    SettingButtonType {
        id: b2
        x: 30
        anchors.top: l2.bottom
        anchors.topMargin: 15
        width: parent.width - 40
        height: 30
        icon.source: "qrc:/images/svg/settings_suggest_black_24dp.svg"
        text: qsTr("Network settings")
        onClicked: {
            UiLogic.goToPage(PageEnum.NetworkSettings)
        }
    }

    // ---------- Server settings ------------
    Rectangle {
        id: l3
        anchors.top: b2.bottom
        anchors.topMargin: 15
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    SettingButtonType {
        id: b3
        x: 30
        anchors.top: l3.bottom
        anchors.topMargin: 15
        width: 330
        height: 30
        icon.source: "qrc:/images/svg/vpn_key_black_24dp.svg"
        text: qsTr("Server Settings")
        onClicked: {
            GeneralSettingsLogic.onPushButtonGeneralSettingsServerSettingsClicked()
        }
    }

    // ---------- Share connection ------------
    Rectangle {
        id: l4
        anchors.top: b3.bottom
        anchors.topMargin: 15
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    SettingButtonType {
        id: b4
        x: 30
        anchors.top: l4.bottom
        anchors.topMargin: 15
        width: 330
        height: 30
        icon.source: "qrc:/images/svg/share_black_24dp.svg"
        text: qsTr("Share connection")
        enabled: GeneralSettingsLogic.pushButtonGeneralSettingsShareConnectionEnable
        onClicked: {
            GeneralSettingsLogic.onPushButtonGeneralSettingsShareConnectionClicked()
        }
    }

    // ---------- Servers ------------
    Rectangle {
        id: l5
        anchors.top: b4.bottom
        anchors.topMargin: 15
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    SettingButtonType {
        id: b5
        x: 30
        anchors.top: l5.bottom
        anchors.topMargin: 15
        width: 330
        height: 30
        icon.source: "qrc:/images/svg/format_list_bulleted_black_24dp.svg"
        text: qsTr("Servers")
        onClicked: {
            UiLogic.goToPage(PageEnum.ServersList)
        }
    }

    // ---------- Add server ------------
    Rectangle {
        id: l6
        anchors.top: b5.bottom
        anchors.topMargin: 15
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }
    SettingButtonType {
        id: b6
        x: 30
        anchors.top: l6.bottom
        anchors.topMargin: 15
        width: 330
        height: 30
        icon.source: "qrc:/images/svg/control_point_black_24dp.svg"
        text: qsTr("Add server")
        onClicked: {
            UiLogic.goToPage(PageEnum.Start)
        }
    }

    Rectangle {
        id: l7
        anchors.top: b6.bottom
        anchors.topMargin: 15
        x: 20
        width: parent.width - 40
        height: 1
        color: "#DDDDDD"
    }


    SettingButtonType {
        x: 30
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        width: 330
        height: 30
        icon.source: "qrc:/images/svg/logout_black_24dp.svg"
        text: qsTr("Exit")
        onClicked: {
            Qt.quit()
        }
    }
}
