import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    property string backupFileName: ""
    property string serverName: ""
    property string serverIp: ""
    property bool isFromSetupWizard: false

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                backButtonVisible: false

                title: qsTr("Backup restored")
                description: {
                    var baseText = qsTr("%1 on \"%2\"").arg(backupFileName).arg(serverName)
                    if (serverIp && serverIp.length > 0) {
                        return baseText + ", " + serverIp
                    }
                    return baseText
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        filled: true

        ButtonType {
            Layout.fillWidth: true

            text: qsTr("To home")

            onClicked: PageController.goToPageHome()
        }

        ButtonType {
            Layout.fillWidth: true

            variant: ButtonType.Secondary

            text: qsTr("To server settings")

            onClicked: {
                PageController.goToPage(PageEnum.PageSettingsServerInfo)
                PageController.goToPageSettingsServerManagement()
            }
        }
    }
}
