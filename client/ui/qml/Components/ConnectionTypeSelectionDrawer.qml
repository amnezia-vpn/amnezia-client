import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType2 {
    id: root

    width: parent.width
    height: parent.height

    expandedStateContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Component.onCompleted: {
            root.expandedHeight = content.implicitHeight + 32
        }

        Header2Type {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("Add new connection")
        }

        LabelWithButtonType {
            id: ip
            Layout.fillWidth: true
            Layout.topMargin: 16

            text: qsTr("Configure your server")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
                PageController.goToPage(PageEnum.PageSetupWizardCredentials)
                root.closeTriggered()
            }
        }

        DividerType {}

        LabelWithButtonType {
            id: qrCode
            Layout.fillWidth: true

            text: qsTr("Open config file, key or QR code")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                root.closeTriggered()
            }
        }

        DividerType {}
    }
}
