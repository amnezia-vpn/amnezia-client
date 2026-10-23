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
        anchors.bottomMargin: 96 + PageController.safeAreaBottomMargin

        Image {
            anchors.centerIn: parent

            width: 153
            height: 131

            source: "qrc:/images/amneziaLogo.svg"
            sourceSize: Qt.size(153 * Screen.devicePixelRatio, 131 * Screen.devicePixelRatio)
            fillMode: Image.PreserveAspectFit
        }
    }

    ButtonType {
        id: startButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

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
