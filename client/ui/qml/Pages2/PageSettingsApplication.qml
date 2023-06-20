import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Application")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Language")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Reset settings and remove all data from the application")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}
        }
    }
}
