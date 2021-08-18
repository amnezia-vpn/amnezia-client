import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 24
        color: "#100A44"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Setup your server to use VPN")
        x: 10
        y: 35
        width: 361
        height: 31
    }
    LabelType {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("If you want easily configure your server just run Wizard")
        wrapMode: Text.Wrap
        x: 40
        y: 100
        width: 301
        height: 41
    }
    LabelType {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Press configure manually to choose VPN protocols you want to install")
        wrapMode: Text.Wrap
        x: 40
        y: 260
        width: 301
        height: 41
    }
    BlueButtonType {
        text: qsTr("Run Setup Wizard")
        y: 150
        width: 301
        height: 40
        anchors.horizontalCenter: parent.horizontalCenter
        onClicked: {
            UiLogic.goToPage(PageEnum.Wizard);
        }
    }
    BlueButtonType {
        text: qsTr("Configure VPN protocols manually")
        y: 310
        width: 301
        height: 40
        anchors.horizontalCenter: parent.horizontalCenter
        onClicked: {
            UiLogic.goToPage(PageEnum.NewServerProtocols);
        }
    }

    Image {
        anchors.horizontalCenter: root.horizontalCenter
        width: GC.trW(150)
        height: GC.trH(22)
        y: GC.trY(590)
        source: "qrc:/images/AmneziaVPN.png"
    }
    ImageButtonType {
        id: back_from_new_server
        x: 10
        y: 10
        width: 26
        height: 20
        icon.source: "qrc:/images/arrow_left.png"
        onClicked: {
            UiLogic.closePage()
        }
    }

}
