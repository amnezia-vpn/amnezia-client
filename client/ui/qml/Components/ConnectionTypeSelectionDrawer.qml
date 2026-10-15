import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

SheetType {
    id: root

    expandedHeight: content.implicitHeight + 16 + PageController.safeAreaBottomMargin

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        SectionHeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("Add new connection")
        }

        ListItemType {
            id: ip
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Configure your server")
            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                PageController.goToPage(PageEnum.PageSetupWizardCredentials)
                root.close()
            }
        }

        ListItemType {
            id: qrCode
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Open config file, key or QR code")
            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                root.close()
            }
        }
    }
}
