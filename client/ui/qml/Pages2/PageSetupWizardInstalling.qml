import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageBase {
    id: root
    page: PageEnum.PageSetupWizardInstalling

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20

                //TODO remove later
                buttonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: "Установка"
                descriptionText: ContainersModel.getCurrentlyInstalledContainerName()
            }
        }
    }
}
