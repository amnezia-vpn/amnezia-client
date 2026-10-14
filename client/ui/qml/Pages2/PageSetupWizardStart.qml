import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root
    enableTimer: (SettingsController.isOnTv()) ? false : true

    Item {
        id: logoArea

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 16 + PageController.safeAreaTopMargin
        anchors.bottomMargin: 104 + PageController.safeAreaBottomMargin

        ColumnLayout {
            anchors.centerIn: parent

            spacing: 0

            Image {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 147
                Layout.preferredHeight: 115

                source: "qrc:/images/amneziaBigLogo.png"
                sourceClipRect: Qt.rect(192, 204, 980, 769)
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }

            CaptionTextType {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 2

                text: "YOUR OWN VPN"
                color: AmneziaStyle.color.textAccent
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
            }
        }
    }

    ButtonType {
        id: startButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        text: qsTr("Let's get started")

        onClicked: {
            PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
        }
    }

    Timer {
        interval: 250
        running: SettingsController.isOnTv()
        repeat: true
        onTriggered: {
            startButton.forceActiveFocus()
            if (startButton.activeFocus) {
                running = false
            }
        }
    }

    onVisibleChanged: {
        if (visible && SettingsController.isOnTv()) {
            startButton.forceActiveFocus()
        }
    }
}
