import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"

Item {
    id: root
    width: GC.screenWidth
    height: GC.screenHeight
    Image {
        x: 20
        y: 424
        width: 325
        height: 1
        source: "qrc:/images/line.png"
    }
    Text {
        x: 20
        y: 440
        width: 281
        height: 21
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 15
        color: "#181922"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        text: qsTr("How to use VPN")
    }
    LabelType {
        id: error_text
        x: 0
        y: 280
        width: 381
        height: 61
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        text: UiLogic.labelErrorText
    }
    Text {
        x: 0
        y: 250
        width: 380
        height: 31
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 15
        color: "#181922"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        text: UiLogic.labelStateText
    }
    Image {
        x: 0
        y: 0
        width: 380
        height: 325
        source: "qrc:/images/background_connected.png"
    }
    BasicButtonType {
        id: button_connect
        x: 150
        y: 200
        width: 80
        height: 40
        checkable: true
        checked: UiLogic.pushButtonConnectChecked
        onCheckedChanged: {
            UiLogic.pushButtonConnectChecked = checked
            UiLogic.onPushButtonConnectClicked(checked)
        }
        background: Image {
            anchors.fill: parent
            source: button_connect.checked ? "qrc:/images/connect_button_connected.png"
                                           : "qrc:/images/connect_button_disconnected.png"
        }
        contentItem: Item {}
        antialiasing: true
        enabled: UiLogic.pushButtonConnectEnabled
    }

    ImageButtonType {
        x: 340
        y: 10
        width: 31
        height: 31
        icon.source: "qrc:/images/settings_grey.png"
        onClicked: {
            UiLogic.goToPage(PageEnum.GeneralSettings)
        }
    }
    BasicButtonType {
        id: button_add_site
        x: 20
        y: 560
        width: 341
        height: 40
        text: qsTr("+ Add site")
        enabled: UiLogic.pushButtonVpnAddSiteEnabled
        background: Rectangle {
            anchors.fill: parent
            radius: 4
            color: {
                if (!button_add_site.enabled) {
                    return "#484952"
                }
                if (button_add_site.containsMouse) {
                    return "#282932"
                }
                return "#181922"
            }
        }

        contentItem: Text {
            anchors.fill: parent
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#D4D4D4"
            text: button_add_site.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        antialiasing: true
        onClicked: {
            UiLogic.goToPage(PageEnum.Sites)
        }
    }
    Item {
        x: 0
        y: 360
        width: 380
        height: 51
        Image {
            x: 311
            y: 10
            width: 15
            height: 15
            source: "qrc:/images/upload.png"
        }
        Image {
            x: 53
            y: 10
            width: 15
            height: 15
            source: "qrc:/images/download.png"
        }
        Text {
            x: 0
            y: 20
            width: 127
            height: 30
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#4171D6"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            text: UiLogic.labelSpeedReceivedText
        }
        Text {
            x: 260
            y: 20
            width: 118
            height: 30
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#42D185"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            text: UiLogic.labelSpeedSentText
        }
    }
    Item {
        x: 20
        y: 470
        width: 351
        height: 91
        enabled: UiLogic.widgetVpnModeEnabled
        RadioButtonType {
            x: 0
            y: 0
            width: 341
            height: 19
            checked: UiLogic.radioButtonVpnModeAllSitesChecked
            text: qsTr("For all connections")
            onCheckedChanged: {
                UiLogic.radioButtonVpnModeAllSitesChecked = checked
                button_add_site.enabled = !checked
                UiLogic.onRadioButtonVpnModeAllSitesToggled(checked)
            }
        }
        RadioButtonType {
            x: 0
            y: 60
            width: 341
            height: 19
            text: qsTr("Except selected sites")
            checked: UiLogic.radioButtonVpnModeExceptSitesChecked
            onCheckedChanged: {
                UiLogic.radioButtonVpnModeExceptSitesChecked = checked
                UiLogic.onRadioButtonVpnModeExceptSitesToggled(checked)
            }
        }
        RadioButtonType {
            x: 0
            y: 30
            width: 341
            height: 19
            text: qsTr("For selected sites")
            checked: UiLogic.radioButtonVpnModeForwardSitesChecked
            onCheckedChanged: {
                UiLogic.radioButtonVpnModeForwardSitesChecked = checked
                UiLogic.onRadioButtonVpnModeForwardSitesToggled(checked)
            }
        }
    }
}
