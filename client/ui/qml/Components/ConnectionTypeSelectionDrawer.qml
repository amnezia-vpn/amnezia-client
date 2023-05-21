import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

Drawer {
    id: root

    edge: Qt.BottomEdge
    width: parent.width
    height: parent.height * 0.4375

    clip: true
    modal: true

    background: Rectangle {
        anchors.fill: parent
        anchors.bottomMargin: -radius
        radius: 16
        color: "#1C1D21"

        border.color: "#2C2D30"
        border.width: 1
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.rightMargin: 16
        anchors.leftMargin: 16

        Header2TextType {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.alignment: Qt.AlignHCenter

            text: "Данные для подключения"
            wrapMode: Text.WordWrap
        }

        LabelWithButtonType {
            id: ip
            Layout.fillWidth: true
            Layout.topMargin: 32

            text: "IP, логин и пароль от сервера"
            buttonImage: "qrc:/images/controls/chevron-right.svg"

            onClickedFunc: function() {
                PageController.goToPage(PageEnum.PageSetupWizardCredentials)
                root.visible = false
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#2C2D30"
        }
        LabelWithButtonType {
            Layout.fillWidth: true

            text: "QR-код, ключ или файл настроек"
            buttonImage: "qrc:/images/controls/chevron-right.svg"

            onClickedFunc: function() {
                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                root.visible = false
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#2C2D30"
        }
    }
}
