import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.Vpn
    logic: VpnLogic

    Image {
        id: bg_top
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        width: parent.width
        height: parent.height * 0.28
        source: "qrc:/images/background_connected.png"
    }

    ImageButtonType {
        x: parent.width - 40
        y: 10
        width: 31
        height: 31
        icon.source: "qrc:/images/settings_grey.png"
        onClicked: {
            UiLogic.goToPage(PageEnum.GeneralSettings)
        }
    }

    AnimatedImage {
        id: connect_anim
        source: "qrc:/images/animation.gif"
        anchors.top: bg_top.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: root.horizontalCenter
        width: Math.min(parent.width, parent.height) / 4
        height: width

        visible: !VpnLogic.pushButtonConnectVisible
        paused: VpnLogic.pushButtonConnectVisible
        //VisibleBehavior on visible { }
    }

    BasicButtonType {
        id: button_connect
        anchors.horizontalCenter: connect_anim.horizontalCenter
        anchors.verticalCenter: connect_anim.verticalCenter
        width: connect_anim.width
        height: width
        checkable: true
        checked: VpnLogic.pushButtonConnectChecked
        onClicked: VpnLogic.onPushButtonConnectClicked()
        background: Image {
            anchors.fill: parent
            source: button_connect.checked ? "qrc:/images/connected.png"
                                           : "qrc:/images/disconnected.png"
        }
        contentItem: Item {}
        antialiasing: true
        enabled: VpnLogic.pushButtonConnectEnabled
        opacity: VpnLogic.pushButtonConnectVisible ? 1 : 0

//        transitions: Transition {
//            NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad; duration: 500 }
//        }
    }


    LabelType {
        id: lb_state
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: button_connect.bottom
        width: parent.width
        height: 21
        horizontalAlignment: Text.AlignHCenter
        text: VpnLogic.labelStateText
    }

    RowLayout {
        id: layout1
        anchors.top: lb_state.bottom
        //anchors.topMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        height: 21


        LabelType {
            Layout.alignment: Qt.AlignRight
            height: 21
            text: qsTr("Server") + ": "
        }

        BasicButtonType {
            Layout.alignment: Qt.AlignLeft
            height: 21
            background: Item {}
            text: VpnLogic.labelCurrentServer
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            onClicked: {
                UiLogic.goToPage(PageEnum.ServersList)
            }
        }
    }

    RowLayout {
        id: layout2
        anchors.top: layout1.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        height: 21


        LabelType {
            Layout.alignment: Qt.AlignRight
            height: 21
            text: qsTr("Service") + ": "
        }

        BasicButtonType {
            Layout.alignment: Qt.AlignLeft
            height: 21
            background: Item {}
            text: VpnLogic.labelCurrentService
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            onClicked: {
                UiLogic.onGotoCurrentProtocolsPage()
            }
        }
    }


    LabelType {
        id: error_text
        anchors.top: layout2.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: 21
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: VpnLogic.labelErrorText
    }

    Item {
        x: 0
        anchors.bottom: line.top
        anchors.bottomMargin: GC.isMobile() ? 0 :10
        width: parent.width
        height: 51
        Image {
            anchors.horizontalCenter: upload_label.horizontalCenter
            y: 10
            width: 15
            height: 15
            source: "qrc:/images/upload.png"
        }
        Image {
            anchors.horizontalCenter: download_label.horizontalCenter
            y: 10
            width: 15
            height: 15
            source: "qrc:/images/download.png"
        }
        Text {
            id: download_label
            x: 0
            y: 20
            width: 130
            height: 30
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#4171D6"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            text: VpnLogic.labelSpeedReceivedText
        }
        Text {
            id: upload_label
            x: parent.width - width
            y: 20
            width: 130
            height: 30
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#42D185"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            text: VpnLogic.labelSpeedSentText
        }
    }

    Rectangle {
        id: line
        x: 20
        width: parent.width - 40
        height: 1
        anchors.bottom: GC.isMobile() ? root.bottom : conn_type_label.top
        anchors.bottomMargin: 10
        color: "#DDDDDD"
    }

    Text {
        id: conn_type_label
        visible: !GC.isMobile()
        x: 20
        anchors.bottom: conn_type_group.top
        anchors.bottomMargin: GC.isMobile() ? 0 :10
        width: 281
        height: GC.isMobile() ? 0: 21
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 15
        color: "#181922"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        text: qsTr("How to use VPN")
    }

    Item {
        id: conn_type_group
        x: 20
        visible: !GC.isMobile()
        anchors.bottom: button_add_site.top
        width: 351
        height: GC.isMobile() ? 0: 91
        enabled: VpnLogic.widgetVpnModeEnabled
        RadioButtonType {
            x: 0
            y: 0
            width: 341
            height: 19
            checked: VpnLogic.radioButtonVpnModeAllSitesChecked
            text: qsTr("For all connections")
            onClicked: VpnLogic.onRadioButtonVpnModeAllSitesClicked(true)
        }
        RadioButtonType {
            x: 0
            y: 60
            width: 341
            height: 19
            text: qsTr("Except selected sites")
            checked: VpnLogic.radioButtonVpnModeExceptSitesChecked
            onClicked: VpnLogic.onRadioButtonVpnModeExceptSitesClicked(true)
        }
        RadioButtonType {
            x: 0
            y: 30
            width: 341
            height: 19
            text: qsTr("For selected sites")
            checked: VpnLogic.radioButtonVpnModeForwardSitesChecked
            onClicked: VpnLogic.onRadioButtonVpnModeForwardSitesClicked(true)
        }
    }

    BasicButtonType {
        id: button_add_site
        visible: !GC.isMobile()
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - 60
        width: parent.width - 40
        height: GC.isMobile() ? 0: 40
        text: qsTr("+ Add site")
        enabled: ! VpnLogic.radioButtonVpnModeAllSitesChecked
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
}
