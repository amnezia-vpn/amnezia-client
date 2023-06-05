import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

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

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Settings")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Servers")
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/server.svg"

                clickedFunction: function() {
                    goToPage(PageEnum.PageSettingsServersList)
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Connection")
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/radio.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Application")
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/app.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Backup")
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/save.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("About AmneziaVPN")
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/amnezia.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}
        }
    }
}
