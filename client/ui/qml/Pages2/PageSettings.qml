import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    FlickableType { // TODO: refactor either replace with ListView or Repeater
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            HeaderType {
                id: header
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Settings")
            }

            LabelWithButtonType {
                id: account
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Servers")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/server.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsServersList)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: connection
                Layout.fillWidth: true

                text: qsTr("Connection")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/radio.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsConnection)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: application
                Layout.fillWidth: true

                text: qsTr("Application")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/app.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsApplication)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: backup
                Layout.fillWidth: true

                text: qsTr("Backup")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/save.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsBackup)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: about
                Layout.fillWidth: true

                text: qsTr("About AmneziaVPN")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/amnezia.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsAbout)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: devConsole
                visible: SettingsController.isDevModeEnabled
                Layout.fillWidth: true

                text: qsTr("Dev console")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/bug.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageDevMenu)
                }
            }

            DividerType {
                visible: SettingsController.isDevModeEnabled
            }

            LabelWithButtonType {
                id: close
                visible: GC.isDesktop()
                Layout.fillWidth: true
                Layout.preferredHeight: about.height

                text: qsTr("Close application")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/x-circle.svg"
                // isLeftImageHoverEnabled: false

                clickedFunction: function() {
                    PageController.closeApplication()
                }
            }

            DividerType {
                visible: GC.isDesktop()
            }
        }
    }
}
