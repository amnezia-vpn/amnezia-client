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

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 0

        Image {
            id: image
            source: "qrc:/images/amneziaBigLogo.png"

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.topMargin: 32 + SettingsController.safeAreaTopMargin
            Layout.preferredWidth: 360
            Layout.preferredHeight: 230
            Layout.bottomMargin: 8
        }

        Item { Layout.fillHeight: true }

        // Dr.Frake VPN — primary login button
        BasicButtonType {
            id: drFrakeLoginButton
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            defaultColor: "#5B4DFF"
            hoveredColor: "#7065FF"
            pressedColor: "#4338CC"
            textColor: AmneziaStyle.color.paleGray

            text: qsTr("Войти в Dr.Frake VPN")

            clickedFunc: function() {
                PageController.goToPage(PageEnum.PageDrFrakeLogin)
            }
        }

        // Manual setup — secondary button
        BasicButtonType {
            id: startButton
            Layout.fillWidth: true
            Layout.bottomMargin: 48 + SettingsController.safeAreaBottomMargin
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            defaultColor: AmneziaStyle.color.transparent
            hoveredColor: AmneziaStyle.color.translucentWhite
            pressedColor: AmneziaStyle.color.sheerWhite
            textColor: AmneziaStyle.color.paleGray
            borderWidth: 1

            text: qsTr("Настроить вручную")

            clickedFunc: function() {
                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
            }
        }
    }
}
