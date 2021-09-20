import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.NewServer
    //logic: {}

    BackButton {
        id: back_from_new_server
    }
    Caption {
        id: caption
        text: qsTr("Setup your server to use VPN")
    }
    LabelType {
        id: labelWizard
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("If you want easily configure your server just run Wizard")
        width: parent.width - 80
        anchors.top: caption.bottom
        anchors.topMargin: 30
    }
    BlueButtonType {
        id: pushButtonWizard
        text: qsTr("Run Setup Wizard")
        anchors.top: labelWizard.bottom
        anchors.topMargin: 10
        onClicked: {
            UiLogic.goToPage(PageEnum.Wizard);
        }
    }
    LabelType {
        id: labelManual
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Press configure manually to choose VPN protocols you want to install")

        width: parent.width - 80
        anchors.top: pushButtonWizard.bottom
        anchors.topMargin: 40
    }

    BlueButtonType {
        text: qsTr("Configure VPN protocols manually")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: labelManual.bottom
        anchors.topMargin: 10
        onClicked: {
            UiLogic.goToPage(PageEnum.NewServerProtocols);
        }
    }

    Logo {
        anchors.bottom: parent.bottom
    }
}
