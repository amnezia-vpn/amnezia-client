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
            source: ""

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.topMargin: 32 + SettingsController.safeAreaTopMargin
            Layout.preferredWidth: 360
            Layout.preferredHeight: 230
            Layout.bottomMargin: 8
        }

        Item { Layout.fillHeight: true }

        // FBLink VPN — only login button
        BasicButtonType {
            id: drFrakeLoginButton
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 48 + SettingsController.safeAreaBottomMargin

            defaultColor: "#00C8FF"
            hoveredColor: "#33D4FF"
            pressedColor: "#0099BB"
            textColor: FBLinkStyle.color.paleGray

            text: qsTr("Войти в FBLink VPN")

            clickedFunc: function() {
                PageController.goToPage(PageEnum.PageFBLinkLogin)
            }
        }

    }
}
